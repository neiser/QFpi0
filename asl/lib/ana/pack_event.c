/* Packing routine
 * 
 * 
 * Revision Date	Reason
 * ------------------------------------------------------------------------
 * 07.05.1997	VH	Copied unpack_event.c
 * 13.05.1997	VH	Included old put_buffer.c
 * 02.10.2001	DH	Modifying Giovanni's version for my own sinister purposes.
 *
 * $Log: pack_event.c,v $
 * Revision 1.6  1998/08/04 08:46:17  hejny
 * define LIBC6 added for such systems
 *
 * Revision 1.5  1998/08/04 08:08:53  hejny
 * bug fixes and changes for Debian 2.0
 *
 * Revision 1.4  1998/05/12 15:25:18  hejny
 * Bugfix: Subevent inserted after closing event
 *
 * Revision 1.3  1998/04/28 08:25:54  hejny
 * Bugfix SE 71 / added SE 82
 *
 * Revision 1.2  1998/04/02 09:59:02  hejny
 * Added Veto-only Modules
 *
 */

static char* rcsid="$Id: pack_event.c,v 1.6 1998/08/04 08:46:17 hejny Exp $";

#define BUFFER_SIZE	4096	// Buffer size in shorts

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "asl.h"		// Library header file
#include "event.h"		// event description and geometry
#include "event_ext.h"	// event description and geometry

#if defined(OSF1) || defined(ULTRIX)
# include <strings.h>
# include <fcntl.h>
# include <sys/types.h>
# if !defined(LIBC6)
#  include <sys/stat.h>
# endif
#elif defined(VMS)
# include <string.h>
# include <stat.h>
# include <types.h>
#endif

#define SECTION "PACK_EVENT"	/* Section name */

#define DEST_NONE		0
#define DEST_FILE		1
#if defined(OSF1) || defined(ULTRIX)
# define DEST_TAPE		2
#endif

extern char current_runname[10];
static char used_runname[10];
static int  data_destination;
static int  PE_fd;
static int  PE_fileopen = 0;

static int  PE_logfile;

#if defined(OSF1) || defined(ULTRIX)
static char PE_tape[20];
static int  PE_max_tape_size;
static int  PE_tape_size;
static ANSI_TAPE*  PE_td;
static int  PE_tapeopen = 0;
#endif

int ana_stop();
void end_pack_event();

/*****************************************************************************
 * 
 * File internal definitions
 * 
 */

static unsigned int 	event_queue[200*BUFFER_SIZE];
static int*		s_event_queue;
static int		queue_pointer = 0;
static int		read_pointer  = 0;

  				       /* Veto Only Modules */
static int		veto_only_det[504];
static int		veto_only_det_num;
  
static int		pack_bgo = 0;

static int		destination;

static char		path[100];
static char		compfilename[10];

void  flush_buffer(char* current_file_name);
int   extract_buffer(unsigned short*);

/*****************************************************************************
 * 
 * Structure of INITFILE:
 *
 * HISTOGRAM ...		Standard entries
 * PICTURE   ...
 *
 * PATH		''		Directory to write 
 *
 * COMPFILE	''		New filename base
 *
 * DEVICE	/dev/rmt0h
 * TAPESIZE	2621440		20 GB (in buffer)
 * 
 * PACK_BGO	0
 * 
 *****************************************************************************/

int init_pack_event()
{
  char    initfile[100];
  FILE    *fp;
  char    line[120];
  char    key[40], arg[80], arg2[40];
  // Status
  printf("<I> Init section: %s\n",SECTION);

#if defined(OSF1) || defined(ULTRIX)
  strcpy(PE_tape,"/dev/rmt0h");
  PE_max_tape_size = 2621440; /* DLT = 20 GB */
#endif
  
  // get initfile
  get_initfile(initfile,SECTION);
  if (initfile[0] == 0) {
    fprintf(stderr,"    No initfile for this section found\n");
    return -1;
  }

  // open initfile
  if ((fp=fopen(initfile,"r"))==NULL) {
    perror("<E> Cannot open initfile");
    fprintf(stderr,"    File = %s\n",initfile);
    return -1;
  }

  // read initfile
  while (fgets( line, sizeof(line), fp)) {
    if (sscanf(line,"%s %s %s",key,arg,arg2)>0) {
      if (!strcmp(key,"HISTOGRAM"))
	   read_h_entry(arg,fp);
      else if (!strcmp(key,"PICTURE"))
	   read_p_entry(arg,fp);

      else if (!strcmp(key,"PATH"))
	   strcpy(path,arg);

      else if (!strcmp(key,"COMPFILE"))
           strcpy(compfilename,arg);

#if defined(OSF1) || defined(ULTRIX)
      else if (!strcmp(key,"DEVICE"))
	   strcpy(PE_tape,arg);
      else if (!strcmp(key,"TAPESIZE"))
	   PE_max_tape_size = atoi(arg);
#endif      
      
      else if (!strcmp(key,"PACK_BGO"))
	   pack_bgo = atoi(arg);

      else
	   line[0] = 0;
      if (line[0]) 
	   printf("    ---> %s",line);					
    } // end: if sscanf(..)
  } // end: if fgets(..)

  fclose(fp);

  add_to_newfile(flush_buffer);
  add_to_endana(end_pack_event);
  
  s_event_queue = (int*) event_queue;
  
  data_destination = DEST_NONE;
  if (!strcmp(ana_def.a_dest,"FILE") )
       data_destination = DEST_FILE;
#if defined(OSF1) || defined(ULTRIX)
  else if (!strcmp(ana_def.a_dest,"TAPE"))
       data_destination = DEST_TAPE;
#endif

  PE_logfile= open("PE_created_files.log", O_WRONLY | O_CREAT | O_APPEND, 0666);
  if (PE_logfile < 0) {
    printf("<E> Can't open logfile\n");
    return -1;
  }
  write(PE_logfile,"-----\n",6);
  
  return 0;
}

/*****************************************************************************
 * 
 * Define global variables to store the histogram IDs to use
 * and fill them.
 * 
 ****************************************************************************/

int book_pack_event()
{
  return 0;
}

/*****************************************************************************
 * 
 * pack_data()
 * 
 *****************************************************************************/

int pack_data()
{
	int event_start;
	int subevent_start;
	int subevent_length;

	int bl, det, i;
	short flags;

/* remember start of event and  
 * leave some free space for the
 * event length and some flags
 */

	event_start = queue_pointer;
	for ( i = 1; i <= 4; i++) event_queue[queue_pointer++] = 0;

	veto_only_det_num = 0;	       // reset number of only veto mods

/* Subevent ID 51: Statistics
 *  event number in run 
 *  valid event number in run
 *  trigger pattern
 */
	subevent_start = queue_pointer++;    // leave space for subevent length
	event_queue[queue_pointer++] = 51;

	event_queue[queue_pointer++] = run_event_number/65536;
	event_queue[queue_pointer++] = run_event_number%65536;
	event_queue[queue_pointer++] = trigger_pattern/65536;
	event_queue[queue_pointer++] = trigger_pattern%65536;
	event_queue[queue_pointer++] = trigger_pattern2/65536;
	event_queue[queue_pointer++] = trigger_pattern2%65536;
	for ( i = 1; i < 7; i++) event_queue[queue_pointer++] = pileup_tdc[i];

	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;
  
/* Subevent ID 60 : TAPS/FW
 *  detector number
 *  energy (100 keV)
 *  time (10 ps)
 *  flags: VETO(1), LED(2)
 */
	subevent_start = queue_pointer++;  // leave space for subevent length
	event_queue[queue_pointer++] = 60;

	for ( bl = 1; bl <= 6; bl++) {
		for ( det = 1; det <= 64; det++) {
			if ( taps[bl].time[det] != -10000) {		//  This is only temporary
				if ( taps[bl].flags[det] & HIT) {
					event_queue[queue_pointer++] = bl*256 + det;
					s_event_queue[queue_pointer++] = 10*taps[bl].e_long[det] + 0.5;
//					s_event_queue[queue_pointer++] = 10*taps[bl].e_short[det] + 0.5;
					s_event_queue[queue_pointer++] = 100*taps[bl].time[det] + 0.5;
					flags = 0;
					if (taps[bl].flags[det] & VETO) flags |= CP_VETO; 
//					if (taps[bl].e_long[det] > taps_geom[bl].led_threshold[det]) flags |= CP_LED;
					if (taps[bl].flags[det] & LED) flags |= CP_LED;
//					if (taps[bl].flags[det] & LED)  flags |= CP_LED_H;
					if (taps_geom[bl].flags[det] & NOT_USABLE) flags |= CP_OFF;
					if (taps_geom[bl].flags[det] & VETO_NOT_USABLE) flags |= CP_VETO_OFF;
					if (taps_geom[bl].flags[det] & NO_PSA) flags |= CP_PSA_OFF;
//					if (taps[bl].flags[det] & TIMEWARP) flags |= CP_TIMEWARP;
					event_queue[queue_pointer++] = flags;
				}
				else if (taps[bl].flags[det] & VETO) {
					veto_only_det_num++;
					veto_only_det[veto_only_det_num] = bl*256 + det;
				}
			}
		}
	}

	for ( det = 1; det <= 142; det++) {
		if ( fdet.time[det] != -10000) {		//  This is only temporary
			if ( fdet.flags[det] & HIT) {
				event_queue[queue_pointer++] = FDET_BLOCK_NUMBER*256 + det;
				s_event_queue[queue_pointer++] = 10*fdet.e_long[det] + 0.5;
//				s_event_queue[queue_pointer++] = 10*fdet.e_short[det] + 0.5;
				s_event_queue[queue_pointer++] = 100*fdet.time[det] + 0.5;
				flags = 0;
				if (fdet.flags[det] & VETO) flags |= CP_VETO; 
//				if (fdet.e_long[det] > fdet_geom.led_threshold[det]) flags |= CP_LED;
				if (fdet.flags[det] & LED) flags |= CP_LED;
//				if (fdet.flags[det] & LED) flags |= CP_LED_H;
				if (fdet_geom.flags[det] & NOT_USABLE) flags |= CP_OFF;
				if (fdet_geom.flags[det] & VETO_NOT_USABLE) flags |= CP_VETO_OFF;
				if (fdet_geom.flags[det] & NO_PSA) flags |= CP_PSA_OFF;
//				if (fdet.flags[det] & TIMEWARP) flags |= CP_TIMEWARP;
				event_queue[queue_pointer++] = flags;
			}
			else if (fdet.flags[det] & VETO) {
				veto_only_det_num++;
				veto_only_det[veto_only_det_num] = FDET_BLOCK_NUMBER*256 + det;
			}
		}
	}
	subevent_length  = (queue_pointer - subevent_start)*2;
	if (subevent_length > 4) event_queue[subevent_start] = subevent_length;
	else queue_pointer = subevent_start;

/* Subevent ID 61: Veto Only Modules
 *  block_number*256 + detector number
 */
	subevent_start = queue_pointer++;  // leave space for subevent length
	event_queue[queue_pointer++] = 61;
    
	for ( i = 1; i <= veto_only_det_num; i++) s_event_queue[queue_pointer++] = veto_only_det[i];
    
	if ( veto_only_det_num%2) s_event_queue[queue_pointer++] = 0;   // Number of shorts must be even
  
	subevent_length = (queue_pointer - subevent_start)*2;
	if ( subevent_length > 4) event_queue[subevent_start] = subevent_length;
	else queue_pointer = subevent_start;

/* Subevent ID 70: Tagger TDC
 *  detector number
 *  time (10 ps)
 */
	subevent_start = queue_pointer++;  // leave space for subevent length
	event_queue[queue_pointer++] = 70;

	event_queue[queue_pointer++] = L_hit_pattern;
	for ( i = 1; i <= tagger_pointer; i++) {
		det = tagger_hit[i];
		event_queue[queue_pointer++] = det;
		s_event_queue[queue_pointer++] = 100.*tagger[det].time + 0.5;
	}
	subevent_length = (queue_pointer - subevent_start)*2;
	if ( subevent_length > 4) event_queue[subevent_start] = subevent_length;
	else queue_pointer = subevent_start;

/* Subevent ID 71: Tagger LEAD-GLAS (BGO)
 *  lead-glas channel 0 
 */
	if (pack_bgo) {
  
		subevent_start = queue_pointer++;    // leave space for subevent length
		event_queue[queue_pointer++] = 71;
  
		event_queue[queue_pointer++] = lead_glas;
		event_queue[queue_pointer++] = 0.;
  
		subevent_length  = (queue_pointer - subevent_start) * 2;
		if (subevent_length > 4) event_queue[subevent_start]  = subevent_length;
		else queue_pointer = subevent_start;
	}

/* Subevent ID 80: SANE
 *  detector number
 *  long energy
 *  long energy no calibration
 *  short energy no calibration
 *  time (10 ps)
 *  flags
 */
  
	subevent_start = queue_pointer++;  // leave space for subevent length
	event_queue[queue_pointer++] = 80;

	for ( i = 1; i <= sane_cell_pointer; i++) {
		det = sane_cell_hits[i];
		if ( sane.flags[det] & SANE_VALID) {
			event_queue[queue_pointer++] = det;
			s_event_queue[queue_pointer++] = 10*sane.long_e[det] + 0.5;
			s_event_queue[queue_pointer++] = 10*sane.long_e_nocal[det] + 0.5;
			s_event_queue[queue_pointer++] = 10*sane.short_e[det] + 0.5;
			s_event_queue[queue_pointer++] = 100.*sane.ctime[det] + 0.5;
			event_queue[queue_pointer++] = sane.flags[det];
		}
	}
	subevent_length  = (queue_pointer - subevent_start)*2;
	if ( subevent_length > 4) event_queue[subevent_start] = subevent_length;
	else queue_pointer = subevent_start;
    
	event_queue[event_start] = (queue_pointer - event_start) - 4;
	event_queue[event_start+1] = 0; 
	event_queue[event_start+2] = 4;
	event_queue[event_start+3] = 0;

	return 0;
}


/*****************************************************************************
 * 
 * pack_scaler()
 * 
 *****************************************************************************/

int pack_scaler()
{
	int event_start;
	int subevent_start;
	int subevent_length;
  
	int i, j, k;

/* remember start of event and  
 * leave some free space for the
 * event length and some flags */

	event_start = queue_pointer;
	for ( i = 1; i <= 4; i++) event_queue[queue_pointer++] = 0;
  
/* Subevent ID 51: Statistics
 *  event number in run 
 *  valid event number in run
 *  trigger pattern (Scaler Event = 0x80000000)
 */
	subevent_start = queue_pointer++;    // leave space for subevent length
	event_queue[queue_pointer++] = 51;

	event_queue[queue_pointer++] = run_event_number/65536;
	event_queue[queue_pointer++] = run_event_number%65536;
	event_queue[queue_pointer++] = trigger_pattern/65536;
	event_queue[queue_pointer++] = trigger_pattern%65536;
	event_queue[queue_pointer++] = trigger_pattern2/65536;
	event_queue[queue_pointer++] = trigger_pattern2%65536;
	for ( i = 1; i < 7; i++) event_queue[queue_pointer++] = pileup_tdc[i];
  
	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;
  
/* Subevent ID 90: Trigger Scaler
 * 72 Scaler:
 *   24 raw
 *   24 accepted
 *   24 downscaled
 */
	subevent_start = queue_pointer++;    // leave space for subevent length
	event_queue[queue_pointer++] = 90;

	for ( i = 1; i < MAX_TRIGGER_SCALER; i++) {
		event_queue[queue_pointer++] = trigger_scaler[i]/65536;
		event_queue[queue_pointer++] = trigger_scaler[i]%65536;
	}
  
	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;


/* Subevent ID 91: Camac Scaler
 * 12 Scaler
 *  1 Interrupts 
 *  2 -
 *  3 -
 *  4 Fast-Clear
 *  5 Accepted X-Trigger
 *  6 P2
 *  7 -
 *  8 -
 *  9 Faraday-Cup
 * 10 -
 * 11 Free-running Scaler
 * 12 Dead-time Inhibited Scaler
 */
	subevent_start = queue_pointer++;    // leave space for subevent length
	event_queue[queue_pointer++] = 91;

	for ( i = 1; i <= 12; i++) {  
		event_queue[queue_pointer++] = camac_scaler[i]/65536;
		event_queue[queue_pointer++] = camac_scaler[i]%65536;
	}
  
	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;

/* Subevent ID 92: Tagger Scaler
 * 352 Scaler
 */
	subevent_start = queue_pointer++;    // leave space for subevent length
	event_queue[queue_pointer++] = 92;

	for ( i = 1; i < MAX_TAGGER; i++) {
		event_queue[queue_pointer++] = ((unsigned int) tagger[i].scaler)/65536;
		event_queue[queue_pointer++] = ((unsigned int) tagger[i].scaler)%65536;
	}
  
	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;

/* Subevent ID 93: SANE Scaler
 * 152 Scaler
 */
	subevent_start = queue_pointer++;  		  // leave space for subevent length
	event_queue[queue_pointer++] = 93;

	for ( i = 1; i < MAX_SANE_SCALER; i++) {
		event_queue[queue_pointer++] = ((unsigned int) sane.scaler[i])/65536;
		event_queue[queue_pointer++] = ((unsigned int) sane.scaler[i])%65536;
	}
  
	event_queue[subevent_start] = (queue_pointer - subevent_start)*2;

	event_queue[event_start] = (queue_pointer - event_start) - 4;
	event_queue[event_start+1] = 0; 
	event_queue[event_start+2] = 4;
	event_queue[event_start+3] = 0;

	return 0;
}



/*****************************************************************************
 *
 * reset_buffer()
 * 
 *****************************************************************************/

void reset_buffer()
{
  queue_pointer = 0;
  read_pointer  = 0;
  return;
}

/*****************************************************************************
 *
 * write_buffer()
 * 
 *****************************************************************************/

void write_buffers()
{

#define	MAX_BUFFERS 12800  
  static int  file_number = 0;
  static char filename[120];
  static int  buffers  = 0;
  static int  run_buffer_number = 0;  
#if defined(OSF1) || defined(ULTRIX)
  static char		input;
  static TAPE_ENTRY 	tentry;
#endif  
  
  unsigned short outbuffer[BUFFER_SIZE];  

  if (data_destination == DEST_NONE) {
    reset_buffer();
    return;
  }
    
  // on new run close old file
  if ( PE_fileopen && strcmp(used_runname,current_runname) ){
    if (data_destination == DEST_FILE) {
      close(PE_fd);
      PE_fileopen = 0;
    }
#if defined(OSF1) || defined(ULTRIX)
    else if (data_destination == DEST_TAPE) {
      t_close(PE_td);
      PE_fileopen = 0;
      PE_tape_size += buffers;
      if (PE_tape_size > PE_max_tape_size) {
	t_umount(PE_td);
	PE_tapeopen = 0;
	PE_td = NULL;
      }
      PE_fileopen = 0;
    }
#endif
  }

  if (!PE_fileopen) {
    if (strcmp(used_runname,current_runname)) {
      file_number = 0;
      run_buffer_number = 0;
      strcpy(used_runname,current_runname);
    }
    file_number++;
    buffers = 0;
    if (data_destination == DEST_FILE) {

//		sprintf(filename,"%s/%s%d.CMD",path, used_runname,file_number);
		sprintf( filename,"%s/%s_%d.CMD", path, compfilename, file_number);

      PE_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (PE_fd < 0) {
	printf("<E> Can't open output file: %s\n",filename);
	ana_stop();
	return;
      }
      printf("<I> New compression disk file opened: %s\n",filename);
    }
#if defined(OSF1) || defined(ULTRIX)
    else if (data_destination == DEST_TAPE) { 
      sprintf(filename,"%s%d.CMD",used_runname,file_number);
      if (!PE_tapeopen) {
	printf("<?> User Request: Insert new labeled tape (y/n) -> ");
	fflush(stdout);
	do { 
	  scanf("%c",&input);
	} while ( (input != 'y') && (input != 'n') );
	if (input == 'n') {
	  ana_stop();
	  return;
	}
	if ((PE_td=t_mount(PE_tape))==NULL) {
	  perror("<E> Cannot mount tape");
	  fprintf(stderr,"    Tape = %s\n",PE_tape);
	  ana_stop();
	  return;
	}
	printf("    Tape %s mounted\n",PE_td->label);
	PE_tapeopen = 1;
	PE_tape_size = 0;
      } /* end mount tape */
      strcpy(tentry.filename,filename);
      tentry.record_format = 'F';
      tentry.block_length  = CBUFFER_SIZE;
      tentry.record_length = CBUFFER_SIZE;
	{
	  int tmp;
      if ((tmp=t_open(PE_td,T_WRITE,&tentry))<0) {
	perror("<E> Error opening tape file, unmounting tape");
	fprintf(stderr,"    File = %s\n",tentry.filename);
	fprintf(stderr,"    Reason = %d\n",tmp);
	t_umount(PE_td);
	PE_tapeopen = 0;
	PE_td = NULL;
	ana_stop();
	return;
      }
	}
      printf("<I> New compression tape file opened: %s\n",tentry.filename);
    }
#endif
    PE_fileopen = 1;
    write(PE_logfile,filename,strlen(filename));
    write(PE_logfile,"\n",1);
  }
  
  while (extract_buffer(outbuffer)>0) {
    buffers++;
    run_buffer_number++;
    outbuffer[6] = run_buffer_number / 65536;
    outbuffer[7] = run_buffer_number % 65536;
    if (data_destination == DEST_FILE) {
      if (write(PE_fd,outbuffer,2*BUFFER_SIZE)<2*BUFFER_SIZE) {
	printf("<E> Error writing buffer\n");
	close(PE_fd);
	PE_fileopen = 0;
	ana_stop();
	return;
      }
    }
#if defined(OSF1) || defined(ULTRIX)
    else if (data_destination == DEST_TAPE) {
      if (t_write(PE_td,(char*) outbuffer,CBUFFER_SIZE) < CBUFFER_SIZE) {
	printf("<E> Error writing buffer\n");
	t_close(PE_td);
	PE_fileopen = 0;
	t_umount(PE_td);
	PE_tapeopen = 0;
	PE_td = NULL;
	ana_stop();
	return;
      }      
    }
#endif
  }
  
  reset_buffer();
  
  if (buffers>MAX_BUFFERS) {
    if (data_destination == DEST_FILE) {
      close(PE_fd);
      PE_fileopen = 0;
    }
#if defined(OSF1) || defined(ULTRIX)
    else if (data_destination == DEST_TAPE) {
      t_close(PE_td);
      PE_fileopen = 0;
      PE_tape_size += buffers;
      if (PE_tape_size > PE_max_tape_size) {
	t_umount(PE_td);
	PE_tapeopen = 0;
	PE_td = NULL;
      }
    }
#endif
  }
    
  return;
}


/*****************************************************************************
 *
 * extract_buffer()
 * 
 *****************************************************************************/

int extract_buffer(unsigned short* outbuffer)
{
  int i;
  int pos;
   
  for (i=0;i<24;outbuffer[i++]=0);
  outbuffer[2] = 4;

  pos = 24;
    
  if (read_pointer>=queue_pointer) return 0;
  
  while( (pos+event_queue[read_pointer]+6)<BUFFER_SIZE ) {
    for (i=read_pointer;i<(read_pointer+event_queue[read_pointer]+4);i++) {
      outbuffer[pos++] = event_queue[i];
    }
    read_pointer += event_queue[read_pointer]+4;
    if (read_pointer>=queue_pointer) break;
  }
    
  outbuffer[pos] = 0xffff;
  outbuffer[4] = pos;
  for(i=pos+1;i<BUFFER_SIZE;outbuffer[i++]=0);
  
  return 1;
}

/*****************************************************************************
 *
 * flush_buffer() @ end of file
 * 
 *****************************************************************************/

void  flush_buffer(char* current_file_name) {

  if (strcmp(used_runname,current_runname)) reset_buffer();

}

/*****************************************************************************
 *
 * end_pack_buffer() @ analysis end
 * 
 *****************************************************************************/

void end_pack_event()
{

  printf("<I> Stop writing buffer\n");

  close(PE_logfile);
  
  if (data_destination == DEST_FILE) {
    if (PE_fileopen) {
      printf("    Close last data file\n");
      close(PE_fd);
      PE_fileopen = 0;
    }
  }
#if defined(OSF1) || defined(ULTRIX)
  else if (data_destination == DEST_TAPE) {  
    if (PE_fileopen) {
      printf("    Close last tape file\n");
      t_close(PE_td);
      PE_fileopen = 0;
    }
    if (PE_tapeopen) {
      printf("    Dismount tape\n");
      t_umount(PE_td);
      PE_tapeopen = 0;
    }
  }
#endif
  
  return;
}
