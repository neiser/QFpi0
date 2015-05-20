/* Unpacking routine
 * 
 * 
 * Revision Date	Reason
 * ------------------------------------------------------------------------
 * 23.02.1995	VH	First commented version
 * 26.04.1995	VH	Start adding raw data Mainz 95
 * 03.05.1995	VH	Mapping of forward detectors added,
 * 			Event Status Register implemented
 * 			valid_event changed to bit in ESR
 * 26.06.1995   VH      Tagger implemented, readout on CFD,VETO and LED
 * 			flag.
 * 23.06.1995   VH	Dynamical bin handling of tagger history spectra
 * 			number of bins are got from tagger.history
 * 07.02.1996	VH	Extracted: Calibration, Geometrie
 *			and filling standard spectra
 * 14.03.1999   MJK	RAW unpacking changed, mainz99 code used
 *
 * 23.11.1999	DLH	SANE crap added
 *
 * 09.12.19999  VH      Microscope added
 *
 * $Log: unpack_event.c,v $
 * Revision 1.4  1998/04/28 08:25:57  hejny
 * Bugfix SE 71 / added SE 82
 *
 * Revision 1.3  1998/04/02 09:59:03  hejny
 * Added Veto-only Modules
 *
 * Revision 1.2  1998/02/06 14:12:51  asl
 * Modules can be switched off reading compressed data
 *
 */

static char* rcsid="$Id: unpack_event.c,v 1.4 1998/04/28 08:25:57 hejny Exp $";

#define BUFFER_SIZE	4096	/* Buffer size in shorts */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "asl.h"		/* Library header file */
#include "event.h"		/* event description and geometrie */
#include "event_ext.h"		/* event description and geometrie */

#if defined(OSF1) || defined(ULTRIX)	/* Platform dependencies */
# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

#define SECTION "UNPACK_EVENT"	/* Section name */

char current_runname[10];

/*****************************************************************************
 * 
 * File internal definitions
 * 
 */

int readout_mode;
int buffer_length_pos = 4; 

static unsigned short multiplicity_laser;
static unsigned short trigger_block_mult;
static int write_scaler = 1;
static int do_softtrigger = 1;

void read_hits(unsigned short*);

void check_new_run(char* current_file_name);
int run_has_changed = 1;       

/*****************************************************************************/

				  	/* Initialize "event.h" */
float pileup_tdc[9];

// SANE
sane_event sane;
short cell_hit_raw;
short veto_hit_raw;
short sane_cell_hits[SANE_NCELL];
short sane_cell_pointer;
short sane_veto_hits[SANE_NVETO];
short sane_veto_pointer;

short sane_tdc_pointer;
short sane_ladc_pointer;
short sane_sadc_pointer;
short sane_vtdc_pointer;
short sane_vadc_pointer;


// TAGGER

int L_hit_pattern;

tagger_event tagger;			
int tagger_latch[MAX_LATCH];
short tagger_hit[MAX_TAGGER];     
short tagger_pointer = 0;
short tagger_wr_hit[MAX_TAGGER];
short tagger_wr_pointer = 0;
unsigned int last_scaler_event = 0;
unsigned int current_scaler_event = 0;
unsigned int last_scaler_event_ana = 0;
unsigned int current_scaler_event_ana = 0;
int camac_scaler[MAX_CAMAC_SCALER];
unsigned int global_scaler[MAX_TOT_SCALER];
float lead_glas;

// Microscope
microscope_event micro;
micro_log_event micro_log;
short micro_hit[MAX_MICROSCOPE];
short micro_pointer = 0; 
short micro_hit_log[MAX_MICRO_LOG];
short micro_pointer_log = 0; 
static int order_in_group[16] = {   16,12,8,4,
                                    15,11,7,3,
                                    14,10,6,2,
                                    13, 9,5,1 };

// Forward Detector
forward_event fdet;		  
forward_hit_buffer fdet_hits;
short fdet_hit_pointer;

// TAPS blocks
block_event taps;
block_hit_buffer taps_hits;
short taps_hit_pointer;      

// LASER event
laser_event laser;

// GEANT event
geant_event geant;

cluster_buffer	cluster;
short cluster_pointer;	// belongs to cluster

primary_particle photon[MAX_CLUS_NUMBER];
int photon_pointer;		// belongs to photon[]

primary_particle neutral[MAX_CLUS_NUMBER];
int neutral_pointer;	// belongs to neutral[]

primary_particle charged[MAX_CLUS_NUMBER];
int charged_pointer;  	// belongs to charged[]
	
primary_particle unknown[MAX_CLUS_NUMBER];
int unknown_pointer; 	// belongs to unknown[]

particle invmass[MAX_PART_NUMBER];
particle dalitz[MAX_PART_NUMBER];
short inv_mult;
short dalitz_mult;
short pi_mult;
short eta_mult;

int event_number;	  	// Event number
int event_valid;	  	// Number of valid events
int event_unsaved;		// Events since last save

int run_event_number;
int run_event_valid;

int events_between_scaler;

unsigned int last_event_fragmented = 0;  
unsigned int current_buffer_number;
unsigned int last_buffer_number;
unsigned int start_buffer_number;
unsigned int start_event_number;
unsigned int events_per_buffer;

unsigned int trigger_pattern;		// Trigger Pattern
unsigned int trigger_pattern2;	
unsigned int trigger_mask_0;		// Mask register for forbidden events
unsigned int trigger_mask_1;		// Mask register for required events
unsigned int trigger_scaler[MAX_TRIGGER_SCALER];
unsigned int num_trigger_scaler;

					// Subevents to be skipped ...
short skip_subevent[SUBEVENT_NUMBERS]; 
int moni_io;	  	// Monitor each ... event
int present_time, old_time;
float e_gamma;	  	// for GEANT simulation

unsigned int event_status = 0;	// Event status register

/*****************************************************************************/

extern int file_has_changed;      // imported from get_buffer.c
extern short veto5_flag, tape_flag;

unsigned short* current_event_start;
int current_event_length;
int buffer_polarisation;
int polarisation_error_occured;

/*****************************************************************************
 * 
 * Structure of INITFILE:
 *
 * READOUT	RAW		(COMPRESSED,GEANT)
 * TRIGGER	0x01111001	(Trigger 1 is the left one)
 * 				0: must not set
 * 				1: must set
 * 				x: set or unset
 * 				only one 0 means no pattern
 *
 * LASER_MULT   1000		Trigger Bit 22 is set if mult > LASER_MULT
 * BLOCK_MULT      2		Software Trigger
 * MONITOR	1000		Show info every 1000. event
 *
 * MIN_SCALER   1900		Minimum events between scaler to compress
 * 				(0: no compression)
 * WRITE_SCALER	   1		Write scaler into compression
 *
 * DO_FRAGMENTATION	0	switch 0/1
 *
 * E_GAMMA	700		Gamma energy GEANT
 *
 * SKIP_SUBEVENT	1	Ignore these subevent numbers
 * SKIP_SUBEVENT	2
 *
 * FILL_BAFS    CFD            (VETO,LED)
 *
 * HISTOGRAM ...		Standard entries
 * PICTURE   ...
 *
 *****************************************************************************/

int init_unpack_event()
{
  char    initfile[100];
  FILE    *fp;
  char    line[120];
  char    key[40], arg[80], arg2[40];
  int	  bl, det, i;

// Status */
  printf("<I> Init section: %s\n",SECTION);

  event_number	        = 0;
  trigger_mask_1        = ~0;
  trigger_mask_0        = 0;
  moni_io	        = 1000;
  control_flags        |= FILL_BAFS_ON_CFD;
  multiplicity_laser    = 1000;
  events_between_scaler = 1950;
  trigger_block_mult    = 2;
  write_scaler		= 1;
  do_softtrigger	= 1;
  
  					/* get initfile */
  get_initfile(initfile,SECTION);
  if (initfile[0] == 0) {
    fprintf(stderr,"    No initfile for this section found\n");
    return -1;
  }

                                        /* open initfile */
  if ((fp=fopen(initfile,"r"))==NULL) {
    perror("<E> Cannot open initfile");
    fprintf(stderr,"    File = %s\n",initfile);
    return -1;
  }

                                        /* read initfile */
  while (fgets( line, sizeof(line), fp)) {
    if (sscanf(line,"%s %s %s",key,arg,arg2)>0) {
      if (!strcmp(key,"HISTOGRAM"))
	   read_h_entry(arg,fp);
      else if (!strcmp(key,"PICTURE"))
	   read_p_entry(arg,fp);
      else if (!strcmp(key,"READOUT")) {
	if (!strcmp(arg,"RAW"))		{
		readout_mode = UNCOMPRESSED;
		buffer_length_pos = 0;
		}
	if (!strcmp(arg,"COMPRESSED"))	readout_mode = COMPRESSED;
	if (!strcmp(arg,"GEANT"))	readout_mode = GEANT;
      }
      else if (!strcmp(key,"TRIGGER")) {
		int i;
      	trigger_mask_0 = 0;
      	trigger_mask_1 = 0;
		for ( i = strlen(arg) - 1; i >= 0; i--)
			if ( arg[i] == '1') trigger_mask_1 |= (1<<i);
			else if ( arg[i]== '0') trigger_mask_0 |= (1<<i);
		     printf( "--------------------------------------------------------\n");
		     printf( "  Trigger                                               \n");
		     printf( "    Key                                                 \n");
		     printf( "                                                        \n");
		     printf( "  TAPS M >= 1   : 25  (Downscaled)                      \n");
		     printf( "  TAPS M >= 2   : 26                                    \n");
		     printf( "  TAPS Block A  : 27             1         2         3  \n");
		     printf( "  SANE Cosmics  : 28    12345678901234567890123456789012\n");
		     printf( "--------------------------------------------------------\n");
      }
      else if (!strcmp(key,"LASER_MULT")) 
	   multiplicity_laser = atoi(arg);
      else if (!strcmp(key,"BLOCK_MULT")) 
	   trigger_block_mult = atoi(arg);
      else if (!strcmp(key,"MIN_SCALER")) 
	   events_between_scaler = atoi(arg);
      else if (!strcmp(key,"WRITE_SCALER")) 
	   write_scaler = atoi(arg);
      else if (!strcmp(key,"DO_SOFTTRIGGER")) 
	   do_softtrigger = atoi(arg);
      else if (!strcmp(key,"MONITOR")) 
	   moni_io = atoi(arg);


      else if (!strcmp(key,"DO_FRAGMENTATION")) {
	if (atoi(arg))	control_flags |= FRAGMENTATION;
	else		control_flags &= (~FRAGMENTATION);
      }
      else if (!strcmp(key,"E_GAMMA")) 
	   e_gamma = atoi(arg);
      else if (!strcmp(key,"SKIP_SUBEVENT")) 
	   skip_subevent[atoi(arg)] = 1;

      else if (!strcmp(key,"FILL_BAFS")) {
        control_flags &= (~( FILL_BAFS_ON_CFD |
			     FILL_BAFS_ON_LED |
			     FILL_BAFS_ON_VETO ));
        if       (!strcmp(arg,"CFD"))  control_flags |= FILL_BAFS_ON_CFD;
        else if  (!strcmp(arg,"LED"))  control_flags |= FILL_BAFS_ON_LED;
        else if  (!strcmp(arg,"VETO")) control_flags |= FILL_BAFS_ON_VETO;
        else                           control_flags |= FILL_BAFS_ON_CFD;
      }
      else
	   line[0] = 0;
      if (line[0]) 
	   printf("    ---> %s",line);					
    } /* end: if sscanf(..) */
  } /* end: if fgets(..) */

  fclose(fp);

  if (!strcmp(ana_def.a_dest,"NONE") ) events_between_scaler = 0;

  // on every new file, check for a new run 
  add_to_newfile(check_new_run);  
  
  return 0;
}

/*****************************************************************************
 * 
 * Define global variables to store the histogram IDs to use
 * and fill them.
 * 
 ****************************************************************************/

int   GB_LENGTH;		
int   STATUS_EVENT;

int   TRIGGER_PATTERN_VALID;
int   TRIGGER_PATTERN_RAW;

int   SUBEVENT_NUM;
int   SUBEVENT_LENGTH[31];

int   TAGGER_SCALER_LENGTH;
int   TAGGER_SETNO;
int   TAGGER_VALUE;

int book_unpack_event()
{

  GB_LENGTH	         = assign_id("buffer.length");
  STATUS_EVENT           = assign_id("event.status");

  TRIGGER_PATTERN_RAW    = assign_id("trigger.pattern.raw");
  TRIGGER_PATTERN_VALID  = assign_id("trigger.pattern.valid");
  
  SUBEVENT_NUM	       = assign_id("event.subevents");
  assign_array("event.subevents.length",SUBEVENT_LENGTH);

  TAGGER_SCALER_LENGTH 	 = assign_id("tagger.scaler.length");
  TAGGER_SETNO = assign_id("tagger.setno");
  TAGGER_VALUE = assign_id("tagger.value");
  
  return 0;
}

/*****************************************************************************
 * 
 * unpack_event()
 * 
 * Input parameter is the pointer to the data buffer. Event number and
 * the position inside the buffer is stored internally.
 * 
 *****************************************************************************/


int unpack_event(short* buffer)
{
	static int buffer_pointer = 24;
	static int last_error_buffer = -1;
	short length;
	int i;
	char temp[100];
	unsigned int low, high;			// for 32 bit word out of 2*16 bit words
	unsigned int last_valid_scaler_event = 0;
  
	last_event_fragmented = 0;

	/* First do all the important things that should be done when a new buffer
	 * starts, i.e. if the buffer_pointer is 24 ! */

	if ( buffer_pointer == 24) {
		
		// get current buffer number and store old
		low  = ((unsigned short*) buffer)[7];
		high = ((unsigned short*) buffer)[6];     	
		last_buffer_number = current_buffer_number;

		if (readout_mode == GEANT) current_buffer_number = high + 65536*low; // grmml
		else current_buffer_number = low + 65536*high;

		start_event_number = event_number;

		// handle a buffer following a fragmented event
		if ((event_status & ESR_FRAGMENTED) && (control_flags & FRAGMENTATION)) {
			if (last_buffer_number != (current_buffer_number - 1)) 
				printf("<W> Fragmented event: buffer %d followed by %d, ignored.\n", last_buffer_number,
						current_buffer_number);
			else {
				if (!buffer[buffer_pointer+1]) printf("<W> Fragmented event followed by unfragmented\n");
				else last_event_fragmented = 1;
			}
		}
	}
  
	// If the last event was valid, set some flags and then if the flag is set, compress it
	if (event_status & ESR_VALID_EVENT) {

		// save old event_status
		if (STATUS_EVENT) { 
			for ( i = 0; i < 32; i++)
				if (event_status & (1<<i)) NHFILL1(STATUS_EVENT, (float) i, 0. , 1.);  
		}

		// add some trigger bits
		if (photon_pointer > 1) trigger_pattern2 |= TB_2_GAMMA;
		if (photon_pointer > 2) trigger_pattern2 |= TB_3_GAMMA;
		if (photon_pointer > 3) trigger_pattern2 |= TB_4_GAMMA;
		if (charged_pointer) trigger_pattern2 |= TB_CHARGED;
		if (neutral_pointer) trigger_pattern2 |= TB_NEUTRAL;
		if (pi_mult) trigger_pattern2 |= TB_PION;
		if (eta_mult) trigger_pattern2 |= TB_ETA;

		// Fill trigger pattern valid
		if (TRIGGER_PATTERN_VALID) {
			for ( i = 0; i < 32; i++)
				if (trigger_pattern & (1<<i)) NHFILL1( TRIGGER_PATTERN_VALID, (float) i+1, 0., 1.);
			for ( i = 0; i < 32; i++)
				if (trigger_pattern2 & (1<<i)) NHFILL1( TRIGGER_PATTERN_VALID, (float) i+33, 0., 1.);
		}

		// do compression
		if (events_between_scaler) {
			if (event_status & ESR_SCALER_EVENT) {
				if (event_status & ESR_VALID_SCALER) {
 					if (event_status & ESR_PACK_EVENT) pack_data();
					if (write_scaler) pack_scaler();
					write_buffers();
					last_valid_scaler_event = run_event_valid;
				}
				else {
					printf("<I> Discard last events and scaler in compression\n");
					reset_buffer();
					run_event_valid = last_valid_scaler_event;
				}
			}
			else if (event_status & ESR_PACK_EVENT) pack_data();
		}
	} // end: last event valid
  

	// Now we loop over all events until we find a new valid one
	do {
		// event length (toi,toi,toi)
		length = buffer[buffer_pointer];
		current_event_start = (unsigned short*) &buffer[buffer_pointer];
		current_event_length = length + 4;

		// Check if this is End of Buffer
		if ( ( length == -1) || ( length == 0) || ( ( buffer_pointer + length) >= BUFFER_SIZE) ||
				( buffer_pointer >= buffer[buffer_length_pos]) || ( buffer[2] != 4) ) {

			NHFILL1( GB_LENGTH, (float) buffer[buffer_length_pos]+UNDIG, 0., 1.);

			buffer_pointer = 24;	       // reset pointer
			events_per_buffer = event_number - start_event_number;
 
			if ( ( event_status  & ESR_FRAGMENTED) && ( control_flags & FRAGMENTATION) ) {
				return 2;		       // end of fragmented buffer
			}
			else { 
				event_status &= (~ESR_VALID_EVENT);
				return 1;		       // indicate End of Buffer
			}
		} // end: check EOB
  

 		/* Count Events only if the event before is not fragmented
 		 * (this is set only if the current event is also fragmented)
 		 */

		if ( !last_event_fragmented) {
   
			event_status = 0;				// reset status register
			if (buffer[buffer_pointer+1]) {	// Check Event fragmentation
				event_status |= ESR_FRAGMENTED;
			}

			event_status |= ESR_PACK_EVENT;    // This is only temporary and packs everything!!!!
      
			// Monitor event number
			if (event_number%moni_io == 0) {
				printf("<I> All: Event Nr. %-10d read, %-10d valid, Buffer Nr. %-10d\n", event_number, event_valid,
						current_buffer_number);
				present_time = time( NULL);
				printf("    Run: Event Nr. %-10d read, %-10d valid, Speed %.0f Events/s\n", run_event_number,
						run_event_valid, ((float) moni_io)/((float) (present_time - old_time)));
				old_time = present_time;
				if (readout_mode == GEANT) {
					printf("    Sim: Event Nr. %-10d\n", geant.event_number); 
				}
			} // end: report event numbers

			// React on new run
			if (run_has_changed) {
				run_event_number = 0;
				run_event_valid  = 0;
				run_has_changed  = 0;
				event_status |= ESR_NEW_RUN;  // set NEW_FILE Flag
				start_buffer_number = current_buffer_number; // remember first buffer of run
			} // end: new run

		       // React on new file
			if (file_has_changed) {
				int current_time;
     
				file_has_changed = 0;			// reset flag from get_buffer.c
				event_status |= ESR_NEW_FILE; 	// set NEW_FILE Flag

					// display event number
				if (event_number) printf("<I> File changed : %d Events, %d valid\n", event_number, event_valid);
				else printf("<I> First file opened\n");
     
					// display run event number
				if (run_event_number) printf("    Run continued: %d Events, %d valid\n", run_event_number, run_event_valid);
				else printf("    New run started\n");

					// display time
				current_time = time( NULL);
				printf("    Time: %s", ctime( &current_time));
     
					// check flow of buffer
				if (last_buffer_number != (current_buffer_number - 1)) {
					if ( (!(event_status & ESR_NEW_RUN)) && ( readout_mode != COMPRESSED)) {
						printf("<W> Buffers are missing between files! (%d followed by %d)\n", last_buffer_number,
								current_buffer_number);
					}
					last_error_buffer = last_buffer_number;
				}
			} // end: file has changed

			else if ( (readout_mode != UNCOMPRESSED) && (ana_def.a_source != "TCPIP")) {
      			       // check flow of buffer
				if ( (last_buffer_number != (current_buffer_number - 1)) && (last_error_buffer != last_buffer_number)) {
					printf("<W> Buffers are missing in file! (%d followed by %d)\n", last_buffer_number,
							current_buffer_number);
					last_error_buffer = last_buffer_number;
				}
			}
     
			// Count events
			event_number++;
			event_unsaved++;
			run_event_number++;
			NHFILL1( STATUS_EVENT,(float) ESR_COUNT_EVENT_BIT,0.,1.);

		} // end: if previous buffer was not fragmented
  
		// Now: unpack buffer and increase buffer pointer
		read_hits( (unsigned short*) (buffer + buffer_pointer));  
		buffer_pointer += length + 4;

		// check trigger pattern
		if ( (trigger_mask_1 && !(trigger_pattern & trigger_mask_1)) || (trigger_pattern & trigger_mask_0))
			event_status &= (~ESR_VALID_EVENT);
  
		// Fill trigger pattern raw
		if (TRIGGER_PATTERN_RAW) {      
			for ( i = 0; i < 32;i++)
				if (trigger_pattern & (1<<i)) NHFILL1( TRIGGER_PATTERN_RAW, (float) i+1, 0., 1.); 
			for ( i = 0; i < 32; i++)
				if (trigger_pattern2 & (1<<i)) NHFILL1( TRIGGER_PATTERN_RAW, (float) i+33, 0., 1.); 
		}
    
		// Call fill_scaler_spectra if scalers are read out
		if ( (event_status & ESR_SCALER_EVENT) && (event_status & ESR_VALID_EVENT)) {
			fill_scaler_spectra();
		}

		// Reset fragmentation
		if (last_event_fragmented) last_event_fragmented = 0; 

	} while (!(event_status & ESR_VALID_EVENT)); // end do loop

	event_valid++;			// count valid events
	run_event_valid++;

	fill_standard_spectra();

	return 0;				// indicate valid event

} // return to analysis loop

/*****************************************************************************
 * 
 * Fills the buffer values in the data structure of the analysis.
 * 
 * Available Modes are:
 * 
 * GEANT	Read GEANT data, one tagger channel is set to E_GAMMA
 * COMPRESSED	First compression of old Mainz data
 * UNCOMPRESSED	Read out mode for Mainz 95
 * 
 ****************************************************************************/

void read_hits( unsigned short* ebuffer)
{
	short* s_ebuffer;
	int bl_hits[7] = { 0, 0, 0, 0, 0, 0, 0};
	int fw_hits = 0;
	int trig = 0;
	int length, pointer, old_pointer;
	int s_len, s_typ, s_end, bl, det_base, i;
//	int veto_pattern, veto_pattern2;   // only for UNCOMPRESSED mode
//	int led_pattern, led_pattern2;     // only for UNCOMPRESSED mode
	unsigned short high, low;
	int det;

	s_ebuffer = (short*) ebuffer;
	event_status |= ESR_VALID_EVENT;     // preset event valid

	// Reset vars for nonfrag. events
	if (!last_event_fragmented) {

		// set all entries to zero
		memset( taps, 0, sizeof( taps));       // taps is an _array_ of struct!
		memset( &fdet, 0, sizeof( fdet));
		memset( &sane, 0, sizeof( sane));
		memset( pileup_tdc, 0, sizeof( pileup_tdc));
		tagger_hit[0] = 0;
		tagger[0].time = -15000.0;		// set one non-coincident tagger
									// real tagger channel starts at 1

		for ( i = 1; i < MAX_TAGGER; tagger[i++].type = 0);
		for ( i = 0; i < MAX_LATCH; tagger_latch[i++] = 0);
		for ( i = 0; i < MAX_MICROSCOPE; micro[i++].flags = 0);
		for ( i = 0; i < SANE_NCELL; sane.raw_flags[i++] = 0);
 
		trigger_pattern = 0;		// triggers patterns
		trigger_pattern2 = 0;
//		veto_pattern = 0;			// veto pattern (RAW)
//		veto_pattern2 = 0;
//		led_pattern = 0;			// led pattern (RAW)
//		led_pattern2 = 0;
		taps_hit_pointer = 0;		// reset hit buffer
		fdet_hit_pointer = 0;		// reset forwards hit buffer
		tagger_pointer = 0;			// reset tagger hit buffer
//		tagger_wr_pointer = 0;		// reset tagger wrong hit buffer
		micro_pointer = 0;			// reset microscope hit buffer
		cluster_pointer = 0;		// reset cluster buffer
		inv_mult = 0;				// reset invariant mass buffer
		L_hit_pattern = 0;			// Tagger Bistro trigger pattern
		lead_glas = 0;
		sane_cell_pointer = 0;		// SANE pointers
		sane_veto_pointer = 0;
		sane_tdc_pointer = 0;
		sane_ladc_pointer = 0;
		sane_sadc_pointer = 0;
		sane_vtdc_pointer = 0;
		sane_vadc_pointer = 0;

		// check for fragmentation
		if (event_status & ESR_FRAGMENTED) { 
			event_status &= ~ESR_VALID_EVENT;
			NHFILL1( STATUS_EVENT,(float) ESR_COUNT_FRAGMENT_BIT,0.,1.);
			if (!(control_flags & FRAGMENTATION)) return;      // Ignore Fragmented Event
		}
	} // end: last event was not fragmented
  
  
	pointer = 0;				// set pointer to start of event buffer
	length  = ebuffer[pointer];	// read event length (toi,toi,toi)
	pointer += 4;				// skip event header

	while ( pointer < length + 4) {	// i.e. the complete event

		if ( (length - pointer) < -2) {	// not enough space for subevent ?
			printf("<E> Not enough space left in event for subevent (pointer %d length %d)\n", pointer, length);
			if (s_ebuffer[pointer]==-1) printf("    ... and we are pointing to -1 !?!\n");
			printf("    This event is set to the last one in this buffer.\n");
			printf("    A EOB (-1) is written to the assumed EOB\n");
			s_ebuffer[length+4] = -1;
			printf("    This is buffer nr. %d start_event %d event %d\n", current_buffer_number, start_event_number,
					event_number);
			pointer = length + 4;
		}
    
/**** UNCOMPRESSED ***********************************************************/

		else if (readout_mode == UNCOMPRESSED) {

			s_len = (ebuffer[pointer++] - 4)/2;			// subevent length
			if ( s_len%2) {
				s_len++;								// for odd cases ;-)
				printf( "<E> odd s_len %d, old s_typ %d\n", s_len, s_typ);
			}
			s_typ = ebuffer[pointer++] & 255;				// subevent type
			if ( s_len == -2) {
				printf( "<E> Subevent length of -2 found: s_typ %d !    TRY TO CONTINUE.\n", s_typ);
				s_len = 0;	 
			}
			s_end = pointer + s_len;						// end of subevent
      
			NHFILL1( SUBEVENT_NUM, (float) s_typ, 0., 1.);
			NHFILL1( SUBEVENT_LENGTH[s_typ], (float) s_len, 0. , 1.);
	
			if ( (s_len == 0) || (skip_subevent[s_typ]) ) {	// skip subevent
//				if (s_len != 0) printf("<E> s_len = %d\n", s_len);
				pointer = s_end;
			}

			else if ( s_typ == 1) {	  
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				trigger_pattern = 65536*low + high;
				trigger_pattern2 = ebuffer[pointer++];
				for ( i = 1; i < 7; i++) pileup_tdc[i] = ebuffer[pointer++];

				pointer = s_end;
			}
						
			else if ( (s_typ > 1) && (s_typ < 14)) {	// TAPS block A,B,C,D,E,F hits
				bl = s_typ/2;						// block number
				det_base = (s_typ%2)*32;				// det offset

				// read veto & led
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				taps[bl].veto_pattern[s_typ%2] = 65536*low + high;
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				taps[bl].led_pattern[s_typ%2] = 65536*low + high;

/*
				for (i=0;i<32;i++) {
					// check VETO
					if ( veto_pattern & (1<<i) ) taps[bl].flags[det_base+i+1] |= VETO;
					// check LED
					if ( led_pattern & (1<<i) ) taps[bl].flags[det_base+i+1] |= LED;
				}
 */  // --> fill_standard_spectra.c
	
				while ( pointer < s_end) {
					det = det_base + ebuffer[pointer++];	// Detector number
					if ( ( det > 0) && ( det < 65)) {	    
						taps[bl].hits++;				// Increase hit counter
						taps[bl].flags[det] |= HIT;		// hit flag
						taps[bl].time[det] = ebuffer[pointer++] + UNDIG;	   // Time
						taps[bl].e_long[det] = ebuffer[pointer++] + UNDIG;	   // Wide
						taps[bl].e_short[det] = ebuffer[pointer++] + UNDIG;	   // Narrow
						if ( taps_hit_pointer < 384) {
							taps_hit_pointer++;						// store hit in hit_buffer
							taps_hits[taps_hit_pointer].block = bl;		// block
							taps_hits[taps_hit_pointer].baf = det;		// detector
						}
						else printf("<W> Too many TAPS hits, Buffer = %d\n", current_buffer_number);
					}
					else {
						printf("<E> TAPS bl %d, det %d\n", bl, det);
						pointer += 3;
					}	  
				}
			} // end s_typ = 2...11  blocks a-f

			// block g1, h1
    			else if ( ( s_typ == 14) || ( s_typ == 16)) {
   
				bl = s_typ - 13;				// block number
				det_base = (bl - 1)*32;			// det offset
				if ( bl == 3) det_base -= 1;

				// read veto & led
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				fdet.veto_pattern[bl-1] = 65536*low + high;	
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				fdet.led_pattern[bl-1] = 65536*low + high;	

/*
				for (i=0;i<31;i++) {
					// check VETO
					if (veto_pattern & (1<<i)) fdet.flags[det_base+i+1] |= VETO;
					// check LED
					if (led_pattern & (1<<i)) fdet.flags[det_base+i+1] |= LED;
				} 
				if (bl==1) {
					if (veto_pattern & (1<<31)) fdet.flags[det_base+32] |= VETO;
					if (led_pattern & (1<<31)) fdet.flags[det_base+32] |= LED;
				}	
*/  // --> fill_standard_spectra.c
	
				while ( pointer < s_end) {
					det = det_base + ebuffer[pointer++];				// Detector number
					if ( ( det > 0) && ( det < 143) && ( det != 95)) {	// H1 only 31
						fdet.hits++;								// Increase hit counter
						fdet.flags[det] |= HIT;						// hit flag
						fdet.time[det] = ebuffer[pointer++] + UNDIG;		// Time
						fdet.e_long[det] = ebuffer[pointer++] + UNDIG;	// Wide
						fdet.e_short[det] = ebuffer[pointer++] + UNDIG;	// Narrow
						if ( fdet_hit_pointer < 142) {
							fdet_hit_pointer++;						// store hit in hit_buffer
							fdet_hits[fdet_hit_pointer].module = det;	// module
						}
						else printf("<W> Too many FDET hits, Buffer = %d\n", current_buffer_number);
						// if (det==64) printf("...unpack %d\n", fdet_hits[fdet_hit_pointer].module);
						
					}
					else {
						if ( det != 95) printf("<E> FW det %d\n", det);
						pointer += 3;
					}	  
				}
			} // end s_typ = 14,16
      
			// block g2, h2
			else if ( ( s_typ == 15) || ( s_typ == 17)) {
				short iled_pattern, iveto_pattern;
	
				iled_pattern = 0;
				iveto_pattern = 0;
				bl = s_typ - 13;				// block number
				det_base = (bl - 1)*32;			// det offset
				if ( bl == 4) det_base -= 2;

				// read veto & led
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				fdet.veto_pattern[bl-1] = 65536*low + high;	
				iveto_pattern = (ebuffer[pointer++] & 0x00ff);
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				fdet.led_pattern[bl-1] = 65536*low + high;	
				iled_pattern = (ebuffer[pointer++] & 0x00ff);
				if (bl==2) {	  
					fdet.veto_pattern[4] |= iveto_pattern;
					fdet.led_pattern[4] |= iled_pattern;
				}
				else {
					fdet.veto_pattern[4] |= (iveto_pattern<<8);
					fdet.led_pattern[4] |= (iled_pattern<<8);
				}

/*
				for (i=0;i<31;i++) {
					// check VETO
					if (veto_pattern & (1<<i)) fdet.flags[det_base+i+1] |= VETO;
					// check LED
					if (led_pattern & (1<<i)) fdet.flags[det_base+i+1] |= LED;
					if (bl==4) {
						if (veto_pattern & (1<<31)) fdet.flags[det_base+32] |= VETO;
						if (led_pattern & (1<<31)) fdet.flags[det_base+32] |= LED;
					}
					if (bl==2) {
						for(i=0;i<8;i++) {
							if (iveto_pattern & (1<<i)) fdet.flags[127+i] |= VETO;
							if (iled_pattern & (1<<i)) fdet.flags[127+i] |= LED;
						}
					}
					else {
						for(i=0;i<8;i++) {
							if (iveto_pattern & (1<<i)) fdet.flags[135+i] |= VETO;
							if (iled_pattern & (1<<i)) fdet.flags[135+i] |= LED;
						}
					}
				}
 */  // --> fill_standard_spectra.c

				while ( pointer < s_end) {
					det = ebuffer[pointer++];						// Detector number
					if ( ( det > 32) && ( bl == 2)) det += 94;
					else if ( ( det > 32) && ( bl == 4)) det += 102;
					else det += det_base;
					if ( ( det > 0) && ( det < 143) && ( det != 64)) {		// G2 only 31
						fdet.hits++;								// Increase hit counter
						fdet.flags[det] |= HIT;						// hit flag
						fdet.time[det] = ebuffer[pointer++] + UNDIG;		// Time
						fdet.e_long[det] = ebuffer[pointer++] + UNDIG;	// Wide
						fdet.e_short[det] = ebuffer[pointer++] + UNDIG;	// Narrow
						if ( fdet_hit_pointer < 142) {
							fdet_hit_pointer++;						// store hit in hit_buffer
							fdet_hits[fdet_hit_pointer].module = det;	// module
						}
						else printf("<W> Too many FDET hits, Buffer = %d\n" ,current_buffer_number);
//						if (det==96) printf("...unpack %d\n", fdet_hits[fdet_hit_pointer].module);
						
					}
					else {
						if ( det != 64) printf("<E> FW detbase %d det %d styp %d\n", det_base,det,s_typ);
						pointer += 3;
					} // det < 143
				}
			} // end s_typ = 15,17

			// Trigger Scaler
			else if ( s_typ == 18 ) {
				if ( !(event_status & ESR_IGNORE_SCALER)) {
					event_status |= ESR_TRIGGER_SCALER_EVENT;
					num_trigger_scaler = 0;
					while ( pointer < s_end) {
						high = ebuffer[pointer++];
						low = ebuffer[pointer++];
						trigger_scaler[++num_trigger_scaler] = low + 65536*high;	// This is different from MK
//						trigger_scaler[++num_trigger_scaler] = 65536*low + high;	// This is what he has
																		// but I think it's wrong.
					}
				}
				pointer = s_end;
			} // end s_typ = 18
	 
			// VIC Subevent
			else if ( s_typ == 25) {
				unsigned short setno;
				int difference, eventno;
				int difference_ana;
				short channel, value;	            	// i.e. Subevent ID
				short check_latch[25];
				int base;

				// printf( "VIC Subevent\n");

				memset( check_latch, 0, sizeof( check_latch));
	  
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				eventno  = 65536*high + low;	  		// Event number

				while ( pointer < s_end) {
	  
					value = s_ebuffer[pointer++];
					setno = ebuffer[pointer++];	// Set number
					NHFILL1(TAGGER_SETNO, (float) setno, 0., 1.);
					NHFILL2(TAGGER_VALUE, (float) setno, (float) value + UNDIG, 1.);
	  
					// Tagger L1, L2 hit pattern
					if ( setno == 0) L_hit_pattern += value;
					else if ( setno == 1) L_hit_pattern += (255 + value);
					else if ( ( setno == 2062) || ( setno == 2126)) {
						// diagnostics
					}	  
	  
					// Microscope TDC (I)
					else if ( ( setno >= 2000) && ( setno <= 2047)) {
						int group = (setno - 2000)/16;
						int mod = (setno - 2000)%16;
						det = order_in_group[mod] + group*16;
						if ( micro[det].flags & HIT) micro[det].time = value + UNDIG;
						else {
							if (micro_pointer < (MAX_MICROSCOPE - 1)) {
								micro_hit[++micro_pointer] = det;
								micro[det].time = value + UNDIG;
								micro[det].energy = -1;
								micro[det].flags |= HIT;
								event_status |= ESR_MICROSCOPE_EVENT;	    
							}
						}
					}
					// Microscope TDC (II)
					else if ( ( setno >= 2064) && ( setno <= 2111)) {
						int group = (setno - 2064)/16 + 3;
						int mod = (setno - 2064)%16;
						det = order_in_group[mod] + group*16;
						if ( micro[det].flags & HIT) micro[det].time = value + UNDIG;
						else {
							if ( micro_pointer < (MAX_MICROSCOPE - 1)) {
								micro_hit[++micro_pointer] = det;
								micro[det].time = value + UNDIG;
								micro[det].energy = -1;
								micro[det].flags |= HIT;
								event_status |= ESR_MICROSCOPE_EVENT;	    
							}
						}
					}
					// Microscope QDC
					else if ( ( setno >= 2200) && ( setno <= 2295)) {
						int group = (setno - 2200)/16;
						int mod = (setno - 2200)%16;
						det = order_in_group[mod] + group*16;
						if ( micro[det].flags & HIT) micro[det].energy = value + UNDIG;
						else {
							if ( micro_pointer < (MAX_MICROSCOPE - 1)) {
								micro_hit[++micro_pointer] = det;
								micro[det].energy = value + UNDIG;
								micro[det].time = -1;
								micro[det].flags |= HIT;
								event_status |= ESR_MICROSCOPE_EVENT;	    
							}
						}
					}

					/**********    SANE     **********/
					//  Veto QDCs
					else if ( ( setno >= 50) && ( setno <= 65)) {
						if ( ( setno == 62)  || ( setno == 64) || (setno == 65)) veto_hit_raw = 0;
						else if ( ( setno == 54) && ( veto5_flag == 0)) veto_hit_raw = 5;  // This is for early data
						else if ( ( setno == 54) && ( veto5_flag == 1)) veto_hit_raw = 0;
						else if ( ( setno == 63) && ( veto5_flag == 1)) veto_hit_raw = 5;  // This is for later data
						else if ( ( setno == 63) && ( veto5_flag == 0)) veto_hit_raw = 0;
						else if ( ( veto5_flag != 0) && ( veto5_flag != 1)) {
							fprintf( stderr, "<E>  We've got big problems because veto5_flag = %d\n", veto5_flag);
							exit( -1);
						}
						else veto_hit_raw = setno - 49;
						if ( veto_hit_raw != 0) {
							if ( sane.raw_flags[veto_hit_raw] & VETO) sane.veto_e[veto_hit_raw] = value + UNDIG;
							else {
								sane_veto_hits[++sane_veto_pointer] = veto_hit_raw;
								sane.veto_e[veto_hit_raw] = value + UNDIG;
								sane.vtime[veto_hit_raw] = -1;
								sane.raw_flags[veto_hit_raw] |= VETO;	    
								event_status |= ESR_SANE_DET_EVENT;	    
							}
				        		++sane_vadc_pointer;
						}
					}

					//  Cell QDCs -- Long Gates I
					else if ( ( setno >= 100) && ( setno <= 195)) {
						if ( ( ( setno >= 100) && ( setno <= 120)) || ( ( setno >= 148) && ( setno <= 170))) {
							if ( setno%2 == 0) cell_hit_raw = (setno - 100)/2 + 1;
							else cell_hit_raw = 0;
						}
						else if ( ( ( setno >= 125) && ( setno <= 147)) || ( ( setno >= 173) && ( setno <= 195))) {
							if ( setno%2 == 1) cell_hit_raw = (setno - 99)/2;
							else cell_hit_raw = 0;
						}
						else cell_hit_raw = 0;

						if ( cell_hit_raw != 0) {
					        	if ( sane.raw_flags[cell_hit_raw] & HIT) sane.long_e[cell_hit_raw] = value + UNDIG;
					        	else {
								sane_cell_hits[++sane_cell_pointer] = cell_hit_raw;
								sane.long_e[cell_hit_raw] = value + UNDIG;
								sane.short_e[cell_hit_raw] = -1;
								sane.ctime[cell_hit_raw] = -1;
								sane.raw_flags[cell_hit_raw] |= HIT;
								event_status |= ESR_SANE_DET_EVENT;
							}
							++sane_ladc_pointer;
						}
					}

					else if ( ( setno >= 200) && ( setno <= 295)) {   //  Cell QDCs -- Short gates I
						if ( ( ( setno >= 200) && ( setno <= 220)) || ( ( setno >= 248) && ( setno <= 270))) {
							if ( setno%2 == 0) cell_hit_raw = (setno - 200)/2 + 1;
							else cell_hit_raw = 0;
						}
						else if ( ( ( setno >= 225) && ( setno <= 247)) || ( ( setno >= 273) && ( setno <= 295))) {
							if ( setno%2 == 1) cell_hit_raw = (setno - 199)/2;
							else cell_hit_raw = 0;
						}
						else cell_hit_raw = 0;			  

						if ( cell_hit_raw != 0) {
					        	if ( sane.raw_flags[cell_hit_raw] & HIT) sane.short_e[cell_hit_raw] = value + UNDIG;
							else {
								sane_cell_hits[++sane_cell_pointer] = cell_hit_raw;
						        	sane.short_e[cell_hit_raw] = value + UNDIG;
								sane.long_e[cell_hit_raw] = -1;
								sane.ctime[cell_hit_raw] = -1;
								sane.raw_flags[cell_hit_raw] |= HIT;	    
								event_status |= ESR_SANE_DET_EVENT;	    
							}
					        	++sane_sadc_pointer;
						}
					}

					//  Cell QDCs -- Long Gates II
					else if ( setno == 362)	cell_hit_raw = 0;
					else if ( ( setno >= 300) && ( setno <= 395)) {       
						if ( ( ( setno >= 300) && ( setno <= 322)) || ( ( setno >= 348) && ( setno <= 370))) {
							if ( setno%2 == 0) cell_hit_raw = (setno - 300)/2 + 49;
							else cell_hit_raw = 0;
						}
						else if ( ( ( setno >= 325) && ( setno <= 347)) || ( ( setno >= 373) && ( setno <= 377))) {
							if ( setno%2 == 1) cell_hit_raw = (setno - 299)/2 + 48;
							else cell_hit_raw = 0;
						}
						else if ( setno == 379) cell_hit_raw = 12;
						else if ( setno == 381) cell_hit_raw = 80;
						else cell_hit_raw = 0;

						if ( cell_hit_raw != 0) {
							if ( sane.raw_flags[cell_hit_raw] & HIT) sane.long_e[cell_hit_raw] = value + UNDIG;
							else {
								sane_cell_hits[++sane_cell_pointer] = cell_hit_raw;
								sane.long_e[cell_hit_raw] = value + UNDIG;
								sane.short_e[cell_hit_raw] = -1;
								sane.ctime[cell_hit_raw] = -1;
								sane.raw_flags[cell_hit_raw] |= HIT;	    
								event_status |= ESR_SANE_DET_EVENT;	    
							}
							++sane_ladc_pointer;
						}
					}

					//  Cell QDCs -- Short Gates II
					else if ( setno == 462) cell_hit_raw = 0;
					else if ( ( setno >= 400) && ( setno <= 495)) {	      
						if ( ( ( setno >= 400) && ( setno <= 422)) || ( ( setno >= 448) && ( setno <= 470))) {
							if ( setno%2 == 0) cell_hit_raw = (setno - 400)/2 + 49;
							else cell_hit_raw = 0;
						}
						else if ( ( ( setno >= 425) && ( setno <= 447)) || ( ( setno >= 473) && ( setno <= 477))) {
							if ( setno%2 == 1) cell_hit_raw = (setno - 399)/2 + 48;
							else cell_hit_raw = 0;
						}
						else if ( setno == 479) cell_hit_raw = 12;
						else if ( setno == 481) cell_hit_raw = 80;
						else cell_hit_raw = 0;			  

						if ( cell_hit_raw != 0) {
							if ( sane.raw_flags[cell_hit_raw] & HIT) {
								sane.short_e[cell_hit_raw] = value + UNDIG;
							}
							else {
								sane_cell_hits[++sane_cell_pointer] = cell_hit_raw;
								sane.short_e[cell_hit_raw] = value + UNDIG;
								sane.long_e[cell_hit_raw] = -1;
								sane.ctime[cell_hit_raw] = -1;
								sane.raw_flags[cell_hit_raw] |= HIT;	    
								event_status |= ESR_SANE_DET_EVENT;	    
							}
							++sane_sadc_pointer;
						}
					}

					//  Cell TDCs
					else if ( ( setno >= 500) && ( setno <= 590)) {
						if ( ( setno >= 500) && ( setno <= 505)) cell_hit_raw = setno - 499;
						else if ( ( setno >= 508) && ( setno <= 577)) cell_hit_raw = setno - 501;
						else if ( ( setno >= 580) && ( setno <= 590)) cell_hit_raw = setno - 503;
						else cell_hit_raw = 0;			  
				
						if ( cell_hit_raw != 0) {
					        	if ( sane.raw_flags[cell_hit_raw] & HIT) sane.ctime[cell_hit_raw] = value + UNDIG;
							else {
								sane_cell_hits[++sane_cell_pointer] = cell_hit_raw;
								sane.ctime[cell_hit_raw] = value + UNDIG;
								sane.long_e[cell_hit_raw] = -1;
								sane.short_e[cell_hit_raw] = -1;
								sane.raw_flags[cell_hit_raw] |= HIT;	    
								event_status |= ESR_SANE_DET_EVENT;	    
							}
					        	++sane_tdc_pointer;
						}
					}

					//  Veto TDCs
					else if ( ( setno >= 596) && ( setno <= 607)) {
						veto_hit_raw = setno - 595;
						if ( sane.raw_flags[veto_hit_raw] & VETO) sane.vtime[veto_hit_raw] = value + UNDIG;
						else {
							sane_veto_hits[++sane_veto_pointer] = veto_hit_raw;
							sane.vtime[veto_hit_raw] = value + UNDIG;
							sane.veto_e[veto_hit_raw] = -1;
							sane.raw_flags[veto_hit_raw] |= VETO;	    
							event_status |= ESR_SANE_DET_EVENT;	    
						}
				        	++sane_vtdc_pointer;
					}
					//*********  End of SANE  **********//

					// Tagger TDC
					else if ( ( setno >= 1200) && ( setno <= 1551)) { 
						channel = 353 - (((setno-1200)&0xffe0) + ((((setno+16)&0x1f)^0x1f) + 1));  // :-) MJK
						if ( ( channel > 0) && ( channel < MAX_TAGGER)) {
							if ( ( tape_flag != 3) || ( ( tape_flag == 3) && ( channel != 66))) { // Shutting off det 66 for tape 3
								tagger_hit[++tagger_pointer] = channel;
								tagger[channel].time   = value + UNDIG;
								tagger[channel].type  |= ( TAGGER_TDC | TAGGER_VALID);
								tagger[channel].energy = tagger[channel].energy_center +
										get_random(-tagger[channel].energy_width/2, tagger[channel].energy_width/2);
							}
						}
					}
					// Tagger Latch Regs
					else if ( ( setno >= 1600) && ( setno <= 1623)) {
						channel = 0x0;
						if ( ( setno >= 1602) && ( setno <= 1623)) { // skipping first two
							if ( ( setno - 1599)%2) {
								if (!check_latch[setno-1601]) {		  
									tagger_latch[setno-1601] = value;
									check_latch[setno-1601] = 1;
								}		
							}	      
							else {
								if (!check_latch[setno-1603]) {		  
									tagger_latch[setno-1603] = value;
									check_latch[setno-1603] = 1;
								}
							}		
						}	    
					}
					// Lead glas
					else if (setno == 23) lead_glas = value + UNDIG;

					// SCALER
	  				  
					/* ATTN: Now set up in following way:
					 * 
					 *   0 - 127  SANE 4 x 4434 CAMAC
					 * 128 - 151  SANE 2 x 2551 CAMAX
					 * 152 - 163  AQCU Trigger Scaler ("camac scaler")
					 * 164 - 259  microscope scaler (same ordering as TDCs)
					 * 260 - 611  tagger scaler (usual ordering)
					 */

					else if ( ( setno == 0xfefe) && ( value == (short) 0xfefe)) {

						int scaler_length = 0;
						int pointer_save;
	    
						high = ebuffer[pointer++];
						low = ebuffer[pointer++];
						scaler_length = 65536*high + low;
	    
						NHFILL1(TAGGER_SCALER_LENGTH, scaler_length, 0., 1.)

						if ( !(event_status & ESR_IGNORE_SCALER)) {

							event_status |= ESR_SCALER_EVENT;
							trigger_pattern |= TB_SCALER;

							// Check gap between events
							last_scaler_event = current_scaler_event;
							current_scaler_event = eventno;
							difference = current_scaler_event - last_scaler_event;
							while ( difference < 0) difference += 65536;
							if (difference != 1000) {
								printf("<I> Irregular Scaler Event. Last %-10d ago\n", difference);
								printf("    Current Event Number is %d/%d (All/Run)\n", event_number,
										run_event_number);
							}

/*
							// Same for TAPS
							last_scaler_event_ana = current_scaler_event_ana;
							current_scaler_event_ana = event_number;
							difference_ana = current_scaler_event_ana - last_scaler_event_ana;
							if (difference_ana != difference) {
								printf("<W> Events missing! Tagger = %d Ev./ TAPS = %d Ev.\n", difference,
										difference_ana);
							}

							if ( (difference > events_between_scaler) && (difference < 2010) )
								event_status |= ESR_VALID_SCALER;
*/
	      
							pointer_save = pointer;

	      					// Fill global array with scaler values
							if ( scaler_length < MAX_TOT_SCALER) {
								for ( i = 0; i < scaler_length; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									global_scaler[i] = low + 65536*high;
								}
								pointer = pointer_save;
							}	      
	      
							if ( scaler_length == 612) {
								event_status |= ESR_VALID_SCALER;

								// SANE Scaler
								for( i = 1; i < MAX_SANE_SCALER; i++) {
								  unsigned int low, high;
								  high  = ebuffer[pointer++];
								  low = ebuffer[pointer++];
								  sane.scaler[i] = low + 65536*high;
								}

								// Camac Scaler
								for ( i = 1; i < 13; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									camac_scaler[i] = low + 65536*high;
								}

								// Microscope Scaler
								for ( i = 1; i < MAX_MICROSCOPE; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									micro[i].last_scaler = micro[i].scaler; 
									micro[i].scaler = low + 65536*high;
								}

								// Tagger Scaler
								for ( i = 1; i < MAX_TAGGER; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									tagger[i].last_scaler = tagger[i].scaler;
									tagger[i].scaler = low + 65536*high;
								}
							}
							else if ( scaler_length == 460) {		
								event_status |= ESR_VALID_SCALER;

								// Camac Scaler
								for ( i = 1; i < 13; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									camac_scaler[i] = low + 65536*high;
								}
								// Microscope Scaler
								for ( i = 1; i < MAX_MICROSCOPE; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									micro[i].last_scaler = micro[i].scaler; 
									micro[i].scaler = low + 65536*high;
								}
								// Tagger Scaler
								for ( i = 1; i < MAX_TAGGER; i++) {
									unsigned int low, high;
									high = ebuffer[pointer++];
									low = ebuffer[pointer++];
									tagger[i].last_scaler = tagger[i].scaler;
									tagger[i].scaler = low + 65536*high;
								}
							}
							else {
								printf("<I> Error: unexpected scaler length: %d\n", scaler_length);
								pointer = pointer_save + 2*scaler_length;
							}
						}
					} // end: Scaler
					else {
						printf("<W> Unexpected SET Number: %d\n",setno);
						printf("    trying to continue VIC subevent.\n");
					}
				} // while index loop
			} // end: if VIC subevent
			else {	
				printf( "<W> Subevent ID not yet implemented, ID = %5d\n", s_typ);
				printf( "    Subevents of this type will be ignored ...\n");
				skip_subevent[s_typ] = 1;
				old_pointer = pointer;
				pointer = s_end;
			}
		} // end: readout mode = UNCOMPRESSED

/**** COMPRESSED *************************************************************/

		// Mainz data , first compression

		else if ( readout_mode == COMPRESSED) {

			s_len = (ebuffer[pointer++] - 4)/2;		// subevent length
			s_typ = ebuffer[pointer++] & 255;			// subevent type
			s_end = pointer + s_len;					// end of subevent

			if (skip_subevent[s_typ]) pointer = s_end; 	// skip subevent

			// Statistics
			else if ( s_typ == 51) {
				unsigned int high, low;
/*				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				run_event_number = 65536*high + low;
				*/
				pointer += 2;
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				trigger_pattern = 65536*high + low;
				high = ebuffer[pointer++];
				low = ebuffer[pointer++];
				trigger_pattern2 = 65536*high + low;
				for ( i = 1; i < 7; i++) pileup_tdc[i] = s_ebuffer[pointer++] + UNDIG;

				// check trigger pattern for absense of valid triggers
				// or presence of forbidden triggers
				if ( (trigger_mask_1 && !(trigger_pattern & trigger_mask_1)) || (trigger_pattern & trigger_mask_0)) {
					event_status &= (~ESR_VALID_EVENT);
					return;
				}
				pointer = s_end;
			}
      
			// All TAPS Blocks
			else if ( s_typ == 60) {
				while ( pointer < s_end) {
					det = ebuffer[pointer++];		// Detector number
					bl = det/256;
					det %= 256;
					if ( bl == FDET_BLOCK_NUMBER) {
						short h_bl, h_det;
						fdet.hits++;				// Increase hit counter
						fdet.flags[det] |= HIT;		// hit flag
						fdet.e_long[det] = (s_ebuffer[pointer++] + UNDIG)/10.;		// Wide
//						fdet.e_short[det] = (s_ebuffer[pointer++] + UNDIG)/10.;	// Narrow
						fdet.time[det] = (s_ebuffer[pointer++] + UNDIG)/100.;		// Time

						if ( det < 33) {
							h_bl = 0;
							h_det = det - 1;
						}
						else if ( det < 64) {
							h_bl = 1;
							h_det = det - 33;
						}
						else if ( det < 95) {
							h_bl = 2;
							h_det = det - 64;
						}
						else if (det < 127) {
							h_bl = 3;
							h_det = det - 95;
						}
						else {
							h_bl = 4;
							h_det = det - 127;
						}
						if (ebuffer[pointer] & CP_VETO) {
//							fdet.flags[det] |= VETO;
							fdet.veto_pattern[h_bl] |= (1<<h_det);
						}
						if (ebuffer[pointer] & CP_LED) {
//							fdet.flags[det] |= LED;
							fdet.led_pattern[h_bl] |= (1<<h_det);
						}
						if (ebuffer[pointer] & CP_OFF) fdet_geom.flags[det] |= NOT_USABLE;
						else if (!(fdet_geom.flags[det] & NOT_USABLE_EXPLICIT)) fdet_geom.flags[det] &= ~NOT_USABLE;
						if (ebuffer[pointer] & CP_PSA_OFF) fdet_geom.flags[det] |= NO_PSA;
						else if (!(fdet_geom.flags[det] & NO_PSA_EXPLICIT)) fdet_geom.flags[det] &= ~NO_PSA;
//						if (ebuffer[pointer] & CP_TIMEWARP) fdet.flags[det] |= TIMEWARP;	  
						pointer++;
						fdet_hit_pointer++;						// store hit in hit_buffer
						fdet_hits[fdet_hit_pointer].module = det;	// detector

					}
					else {
						taps[bl].hits++;					// Increase hit counter
						taps[bl].flags[det] |= HIT;			// hit flag
						taps[bl].e_long[det] = (s_ebuffer[pointer++] + UNDIG)/10.;	// Wide
//						taps[bl].e_short[det] = (s_ebuffer[pointer++] + UNDIG)/10.;	// Narrow
						taps[bl].time[det] = (s_ebuffer[pointer++] + UNDIG)/100.;		// Time
						if (ebuffer[pointer] & CP_VETO) {
//							taps[bl].flags[det] |= VETO;
							taps[bl].veto_pattern[(det-1)/32] |= (1<<((det-1)%32));
						}
						if (ebuffer[pointer] & CP_LED) {
//							taps[bl].flags[det] |= LED;
							taps[bl].led_pattern[(det-1)/32] |= (1<<((det-1)%32));
						}
						if (ebuffer[pointer] & CP_OFF) taps_geom[bl].flags[det] |= NOT_USABLE;
						else if (!(taps_geom[bl].flags[det] & NOT_USABLE_EXPLICIT))
							taps_geom[bl].flags[det] &= ~NOT_USABLE;
						if (ebuffer[pointer] & CP_VETO_OFF) taps_geom[bl].flags[det] |= VETO_NOT_USABLE;
						else if (!(taps_geom[bl].flags[det] & VETO_NOT_USABLE_EXPLICIT))
							taps_geom[bl].flags[det] &= ~VETO_NOT_USABLE;
						if (ebuffer[pointer] & CP_PSA_OFF) taps_geom[bl].flags[det] |= NO_PSA;
						else if (!(taps_geom[bl].flags[det] & NO_PSA_EXPLICIT)) taps_geom[bl].flags[det] &= ~NO_PSA;
//						if (ebuffer[pointer] & CP_TIMEWARP) taps[bl].flags[det] |= TIMEWARP;	  
						pointer++;
						taps_hit_pointer++;						// store hit in hit_buffer
						taps_hits[taps_hit_pointer].block = bl;		// block
						taps_hits[taps_hit_pointer].baf = det;		// detector
					}	   
				}
			}  //  End All TAPS Blocks

			// Veto Only Modules
			else if ( s_typ == 61) {
				while ( pointer < s_end) {
					bl = s_ebuffer[pointer]/256;
					det = s_ebuffer[pointer++]%256;
					if ( bl == FDET_BLOCK_NUMBER) {
						short h_bl, h_det;
//						fdet.flags[det] |= VETO;
						if ( det < 33) {
							h_bl = 0;
							h_det = det - 1;
						}
						else if ( det < 64) {
							h_bl = 1;
							h_det = det - 33;
						}
						else if (det < 95) {
							h_bl = 2;
							h_det = det - 64;
						}
						else if (det < 127) {
							h_bl = 3;
							h_det = det - 95;
						}
						else {
							h_bl = 4;
							h_det = det - 127;
						}
						fdet.veto_pattern[h_bl] |= (1<<h_det);
					}
					else {	     
//						taps[bl].flags[det] |= VETO;
						taps[bl].veto_pattern[(det-1)/32] |= (1<<((det-1)%32));
					}	   
				}
			}

			// Tagger TDC
			else if ( s_typ == 70) {
				L_hit_pattern = s_ebuffer[pointer++];
				while ( pointer < s_end) {
					det = ebuffer[pointer++];
					if ( ( tape_flag != 3) || ( ( tape_flag == 3) && ( det != 66))) {  // Shutting off det 66 for tape 3
						tagger_hit[++tagger_pointer] = det;
						tagger[det].time = (s_ebuffer[pointer++] + UNDIG)/100.;
						tagger[det].type |= ( TAGGER_TDC | TAGGER_VALID);
						tagger[det].energy  = tagger[det].energy_center +
			                         get_random( -tagger[det].energy_width/2, tagger[det].energy_width/2);
					}
					else pointer++;
				}
			}

			// Tagger Lead Glas
			else if ( s_typ == 71) {
				while ( pointer < s_end) {
					lead_glas = (ebuffer[pointer++] + UNDIG);
					pointer++;
				}
			}

			//  SANE
			else if ( s_typ == 80) {
				while ( pointer < s_end) {
					det = ebuffer[pointer++];													// Detector number
					sane.long_e[det] = (s_ebuffer[pointer++] + UNDIG)/10.;			// Long E
					sane.long_e_nocal[det] = (s_ebuffer[pointer++] + UNDIG)/10.;	// Long E No Cal
					sane.short_e[det] = (s_ebuffer[pointer++] + UNDIG)/10.;			// Short E No Cal
					sane.ctime[det] = (s_ebuffer[pointer++] + UNDIG)/100.;			// Time
					sane.flags[det] = ebuffer[pointer++];									// Flags
					sane_cell_hits[++sane_cell_pointer] = det;
				}
			}

			// Trigger Scaler
			else if ( s_typ == 90) {
				unsigned int low, high;
				event_status |= ESR_TRIGGER_SCALER_EVENT;
				num_trigger_scaler = 0;
				while ( pointer < s_end) {
					high = ebuffer[pointer++];
					low  = ebuffer[pointer++];
//					trigger_scaler[++num_trigger_scaler] = 66536*low + high;	//  order switched --> bug in readout
					trigger_scaler[++num_trigger_scaler] = low + 66536*high;	// Again, I think this line is right.
				}
				pointer = s_end;
			}

			// Camac Scaler
			else if ( s_typ == 91) {
				unsigned int low, high;
				det = 0;
				while ( pointer < s_end) {
					high = ebuffer[pointer++];
					low  = ebuffer[pointer++];
					camac_scaler[++det] = low + 65536*high;
				}
				pointer = s_end;
			}

		 	// Tagger Scaler
			else if ( s_typ == 92) {
				unsigned int low, high;
				event_status |= (ESR_SCALER_EVENT | ESR_VALID_SCALER);
				trigger_pattern |= TB_SCALER;
				det = 0;
				while ( pointer < s_end) {
					det++;
					high = ebuffer[pointer++];
					low  = ebuffer[pointer++];
					tagger[det].last_scaler = tagger[det].scaler;
					tagger[det].scaler = low + 65536*high;
				}
				pointer = s_end;
			}

			//  SANE Scaler
			else if ( s_typ == 93) {
				unsigned int low, high;
				det = 0;
				while ( pointer < s_end) {
					high = ebuffer[pointer++];
					low = ebuffer[pointer++];
					sane.scaler[++det] = low + 65536*high;
				}
				pointer = s_end;
			}

			else {	
				printf( "<W> Subevent ID not yet implemented, ID = %5d\n", s_typ);
				printf( "    Subevents of this type will be ignored ...\n");
				skip_subevent[s_typ] = 1;
				pointer = s_end;
			}
	
		} // end: readout mode = compressed

/**** GEANT ******************************************************************/

		// GEANT simulation data
		else if ( readout_mode == GEANT) {

			s_typ = ebuffer[pointer++] & 255;		// subevent length
			s_len = ebuffer[pointer++];				// subevent type
			s_end = pointer+s_len;						// end of subevent

			trigger_pattern = ~0;						// all triggers fired 

			NHFILL1( SUBEVENT_NUM, (float) s_typ, 0., 1.);
			NHFILL1( SUBEVENT_LENGTH[s_typ], (float) s_len, 0. , 1.);

			// skip subevent
			if (skip_subevent[s_typ]) pointer = s_end;
      
			else if ( s_typ == 1) {							// GEANT event number
				unsigned short low, high;
				low = ebuffer[pointer++];
				high = ebuffer[pointer++];
				geant.event_number = low + 65536*high;
				pointer = s_end;
			}
      
			else if ( s_typ == 2) {						// input kinematmics
				geant.particle_number = ebuffer[pointer++];
				for ( i = 1; i <= geant.particle_number; i++) {
					geant.id[i] = ebuffer[pointer++];
					geant.x[i]  = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.y[i]  = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.z[i]  = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.px[i] = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.py[i] = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.pz[i] = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.theta[i] = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					geant.phi[i]   = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
				}
				pointer = s_end;
			}

			// Tagger TDC
			else if ( s_typ == 3) {
				while ( pointer < s_end) {
					det = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					tagger_hit[++tagger_pointer] = det;
					tagger[det].time = 0.0;
					tagger[det].type |= ( TAGGER_TDC | TAGGER_VALID);
//					tagger[det].energy  = (ebuffer[pointer++] + UNDIG)/10. - 3276.8;
					tagger[det].energy  = tagger[det].energy_center + get_random(-tagger[det].energy_width/2,
			                     tagger[det].energy_width/2);
				}
			}

			// BAF Blocks
			else if ( (s_typ >= 11) && (s_typ <= 10 + BLOCKNUMBER)) {
				s_typ -= 10;
				while ( pointer < s_end) {
					det = ebuffer[pointer++];		// detector number
					taps[s_typ].hits++;				// increase hits
					taps[s_typ].flags[det]  |= HIT;	// hit flag set
					taps[s_typ].e_long[det] = (ebuffer[pointer++] + UNDIG)/10.;
					taps[s_typ].time[det]   = (ebuffer[pointer++] + UNDIG)/100.;
					taps[s_typ].time[det]  -= taps_geom[s_typ].abs[det]/29.98;
					taps_hit_pointer++;				// store hit in hit_buffer
					taps_hits[taps_hit_pointer].block = s_typ;	// block
					taps_hits[taps_hit_pointer].baf   = det;	// detector
					pointer++;							// are we skipping something here???
				}
			}

			// Vetos
			else if ( (s_typ >= 21) && (s_typ <= 20 + BLOCKNUMBER)) {
				s_typ -= 20;
				while ( pointer < s_end) {
					det = ebuffer[pointer++];
					if ( !(taps_geom[s_typ].flags[det] & VETO_NOT_USABLE)) {
						taps[s_typ].flags[det]   |= VETO;	 // set veto flag
						taps[s_typ].veto_pattern[(det-1)/32] |= (1<<((det-1)%32));
					}
					pointer += 3;							// are we skipping something here???
				}
			}
      
			else if ( s_typ == 40) {			// Forward Wall vetoes
				short h_bl, h_det;
				while ( pointer < s_end) {
					det = ebuffer[pointer++];	// detector number
					pointer++;				// skip plastic energy
					pointer++;				// skip plastic time
/*					if ( !(fdet.flags[det] & HIT)) {
						fdet.hits++;
						fdet.flags[det] |= HIT;
						fdet.time[det] = (ebuffer[pointer++] + UNDIG)/100.;
						fdet.time[det] -= fdet_geom.abs[det]/29.98;
						fdet_hit_pointer++;
						fdet_hits[fdet_hit_pointer].module = det;
					}
					else {
						fdet.time[det] = (ebuffer[pointer++] + UNDIG)/100.;
						fdet.time[det] -= fdet_geom.abs[det]/29.98;
					}
*/
	  
					if ( det < 33) {
						h_bl = 0;
						h_det = det - 1;
					}
					else if (det<64) {
						h_bl = 1;
						h_det = det - 33;
					}
					else if (det<95) {
						h_bl = 2;
						h_det = det - 64;
					}
					else if (det<127) {
						h_bl = 3;
						h_det = det - 95;
					}
					else {
						h_bl = 4;
						h_det = det - 127;
					}
					if (!(fdet_geom.flags[det]&VETO_NOT_USABLE)) {
//						fdet.flags[det]   |= VETO;	  // set veto flag
//						fdet.veto_pattern[(det-1)/32] |= (1<<((det-1)%32));
						fdet.veto_pattern[h_bl] |= (1<<h_det);
					}	  
					pointer++;
				}
			}

      		// Forward BaF2
			else if ( s_typ == 41) {
				while ( pointer < s_end) {
					det = ebuffer[pointer++];				// detector number
					fdet.e_long[det] += (ebuffer[pointer++] + UNDIG) / 10.;
					fdet.hits++;				// increase hits
					fdet.flags[det] |= HIT;		// set HIT flag
					fdet.time[det] = (ebuffer[pointer++] + UNDIG) / 100.;
					fdet.time[det] -= fdet_geom.abs[det]/29.98;
					fdet_hit_pointer++;
					fdet_hits[fdet_hit_pointer].module = det;
					pointer++;
				}
			}

// *******************************************************
//  This is totally new as of 27.05.03 --- DLH
//  It is not working yet because i still have
//  to rewrite the geant part.
      		// SANE
			else if ( s_typ == 51) {
				while ( pointer < s_end) {
					det = ebuffer[pointer++];												// Detector number
					sane.long_e[det] = (ebuffer[pointer++] + UNDIG)/10.;			// Long E
					sane.ctime[det] = (ebuffer[pointer++] + UNDIG)/100.;			// Time
					sane_cell_hits[++sane_cell_pointer] = det;
					sane.flags[det] |= SANE_VALID;										// Flags
					sane.flags[det] |= SANE_VETO;											// Flags
					pointer++;																	// This should skip the particle ID
				}
			}
// End of new section
// *******************************************************

  			else {
				printf( "<W> Subevent ID not yet implemented, ID = %5d\n", s_typ);
				printf( "    Subevents of this type will be ignored ...\n");
				skip_subevent[s_typ] = 1;
				pointer = s_end;
			}


			// set gamma energy
/*			if (!tagger_pointer) {
				tagger_hit[++tagger_pointer] = GEANT_TAGGER;
				tagger[GEANT_TAGGER].time = 0.0;
				tagger[GEANT_TAGGER].type |= ( TAGGER_VALID | TAGGER_TDC | TAGGER_LATCH );
				tagger[GEANT_TAGGER].energy = e_gamma;
				tagger[GEANT_TAGGER].energy_width = 0.5;
				tagger[GEANT_TAGGER].time_win_coin_low = -1.0;
				tagger[GEANT_TAGGER].time_win_coin_high =  1.0;
			}
*/
		} // End GEANT readout

	} // End Readout_mode (while)


	// only use valid microscope channels
	{
		int i, j, det;
		j = 0;
		for( i = 1; i <= micro_pointer; i++) {
			det = micro_hit[i];
			if ( (micro[det].time > 0.) && (micro[det].time < 4090)) micro_hit[++j] = det;
		}
		micro_pointer = j;
	}

	// Calibrate the data if neccesary
	if ( (control_flags & CALIBRATE) && (event_status & ESR_VALID_EVENT) ) {

		process_docal();

		if ( do_softtrigger) {
			for ( i = 1; i <= taps_hit_pointer; i++) {
				bl = taps_hits[i].block;
				det = taps_hits[i].baf;
				if (taps[bl].e_long[det] > taps_geom[bl].led_threshold[det]) bl_hits[bl]++;
			}
  
			for ( i = 1; i <= fdet_hit_pointer; i++) {
				det = fdet_hits[i].module;
				if (fdet.e_long[det] > fdet_geom.led_threshold[det]) fw_hits++;
			}
  
			for ( bl = 1; bl < 7; bl++) if (bl_hits[bl]) trig++;
			if ( fw_hits) trig++;
			if ( trig >= trigger_block_mult) trigger_pattern |= TB_SOFTTRIGGER ;
		}
	}

	return;
}

/*****************************************************************************
 * 
 * Check for new run
 * 
 */

void check_new_run( char* current_filename)
{
	static int init_check = 1;

	if ( init_check) {
		init_check = 0;
		strncpy( current_runname, current_filename, 9);
		current_runname[9] = 0;
		if ( isdigit( current_runname[8])) current_runname[8]=0;
	}
	else {
		if ( strncmp( current_runname, current_filename, strlen( current_runname))) {
			run_has_changed = 1;
			strncpy( current_runname, current_filename, 9);
			current_runname[9] = 0;
			if ( isdigit( current_runname[8])) current_runname[8]=0;
		}
	}
  
	return;
}
