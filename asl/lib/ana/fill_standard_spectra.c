/* Filling Standard Spectra
 * 
 * 
 * Revision Date	Reason
 * ------------------------------------------------------------------------
 * 07.02.1996	VH	Extracted from unpack_event.c
 * 17.06.1997	VH	Added fill_scaler_spectra
 * 20.01.1998	VH	Added control spectra
 * 06.04.1999	MJK	FW changed for MAINZ99 setup, (VETOs and FILLING)
 * 
 * 09.12.1999   VH	Added Microscope
 * 07.03.2001   DH	Added SANE Scaler Stuff
 * 12.04.2001   DH	Changed the p2tos stuff
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "asl.h"			// Library header file
#include "event.h"			// event description and geometry
#include "event_ext.h"		// event description and geometry

#if defined(OSF1) || defined(ULTRIX)	/* Platform dependencies */
# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

#define SECTION "STANDARD_SPECTRA"	// Section name

/*****************************************************************************/

extern int DEBUG;

// Initialize "event.h"
	
int             current_position = -1;

/*****************************************************************************
 * 
 * Structure of INITFILE:
 *
 * DUMP_SCALER  0		(0/1) switch on/off
 * DUMP_FILE	Filename
 *
 * HISTOGRAM ...		Standard entries
 * PICTURE   ...
 *
 ****************************************************************************/

int init_standard_spectra()
{
	char initfile[100];
	char line[120];
	char key[40], arg[80], arg2[40];

	FILE *fp;

	// Status
	printf( "<I> Init section: %s\n", SECTION);

	// get initfile
	get_initfile( initfile, SECTION);
	if (initfile[0] == 0) {
		fprintf( stderr,"    No initfile for this section found\n");
		return -1;
	}

	// open initfile
	if ( ( fp = fopen( initfile, "r")) == NULL) {
		perror("<E> Cannot open initfile");
		fprintf(stderr,"    File = %s\n", initfile);
		return -1;
	}

	// read initfile
	while (fgets( line, sizeof(line), fp)) {
		if (sscanf(line,"%s %s %s",key,arg,arg2)>0) {
			if (!strcmp(key,"HISTOGRAM")) read_h_entry(arg,fp);
			else if (!strcmp(key,"PICTURE")) read_p_entry(arg,fp);
			else line[0] = 0;

			if (line[0]) printf("    ---> %s",line);					
		}
	}

	fclose( fp);

#if defined(VMS)
	{
		char help[100];
		sprintf( help, "%s;1", dump_file);
		strcpy( dump_file, help);
	}
#endif  

	return 0;
}

/*****************************************************************************
 * 
 * Define global variables to store the histogram IDs to use
 * and fill them.
 * 
 ****************************************************************************/

int   PILEUP_TDC_SPEC[9];
int   PILEUP_25;
int   L_HIT_PATTERN;
int   T_WIDE[BLOCKNUMBER][BAFNUMBER];  
//int   T_NARROW[BLOCKNUMBER][BAFNUMBER];
//int   T_PSA[BLOCKNUMBER][BAFNUMBER];  
int   T_TIME[BLOCKNUMBER][BAFNUMBER];  
//int   T_PSA_2D[BLOCKNUMBER][BAFNUMBER];
//int   T_PSA_2D_ALL;
int   T_TIME_2D[BLOCKNUMBER][BAFNUMBER];
int   T_TIME_2D_ALL;
int   T_PATTERN_BAF[BLOCKNUMBER];
int   T_PATTERN_VETO[BLOCKNUMBER];
int   T_PATTERN_LED[BLOCKNUMBER];
int   T_MULT[BLOCKNUMBER];
int   T_PATTERN_BAF_ALL;
int   T_PATTERN_VETO_ALL;
int   T_PATTERN_LED_ALL;
int   T_PATTERN_VETO_NOCFD;
int   T_PATTERN_VETO_CFD;
int   T_PATTERN_LED_NOCFD;
int   T_PATTERN_CFD_NOWIDE;
//int   T_PATTERN_CFD_NONARROW;
int   T_PATTERN_CFD_NOTIME;
int   T_PATTERN_CFD_FULLWIDE;
//int   T_PATTERN_CFD_FULLNARROW;
int   T_PATTERN_CFD_FULLTIME;
int   T_MULT_ALL;
int   T_BAF_THETA;
int   T_BAF_PHI;
int   T_BAF_GEOM;
int   T_LED_GEOM;
int   T_VETO_THETA;
int   T_VETO_PHI;
int   T_VETO_GEOM;
int   T_VETO_B[BAFNUMBER];
int   T_VETO_E[BAFNUMBER];
int   LED_MULT;
int   VETO_MULT;
int   LED_STAT;

int   F_WIDE[MAX_FORWARD];
//int   F_NARROW[MAX_FORWARD];
//int   F_PSA[MAX_FORWARD];
int   F_TIME[MAX_FORWARD];
int   F_PATTERN_BAF;
int   F_PATTERN_LED;
int   F_PATTERN_VETO;
int   F_PATTERN_VETO_NOCFD;
int   F_PATTERN_VETO_CFD;
int   F_PATTERN_LED_NOCFD;
int   F_PATTERN_CFD_NOWIDE;
//int   F_PATTERN_CFD_NONARROW;
int   F_PATTERN_CFD_NOTIME;
int   F_PATTERN_CFD_FULLWIDE;
//int   F_PATTERN_CFD_FULLNARROW;
int   F_PATTERN_CFD_FULLTIME;
int   F_MULT;
int   F_BAF_THETA;
int   F_BAF_PHI;
int   F_PSA_2D[MAX_FORWARD];
int   F_PSA_2D_ALL;
int   F_TIME_2D[MAX_FORWARD];
int   F_TIME_2D_ALL;
int   F_VETO_THETA;
int   F_VETO_PHI;
int   F_VETO_GEOM;

int   TOTAL_MULT;

int   PATTERN_CFD;
int   PATTERN_CFD_NOWIDE;
//int   PATTERN_CFD_NONARROW;
int   PATTERN_CFD_NOTIME;
int   PATTERN_CFD_FULLWIDE;
//int   PATTERN_CFD_FULLNARROW;
int   PATTERN_CFD_FULLTIME;
int   PATTERN_LED;
int   PATTERN_LED_NOCFD;
int   PATTERN_VETO;
int   PATTERN_VETO_NOCFD;
int   PATTERN_VETO_CFD;

int   TAGGER_MULT;
int   TAGGER_ENERGY;
int   TAGGER_CHANNEL;
int   TAGGER_CHANNEL_T1;
int   TAGGER_TIME;
int   TAGGER_TIME_SINGLE[MAX_TAGGER];
int   TAGGER_SCALER;
int   TAGGER_SCALER_SUM;
int   TAGGER_SCALER_RATE;
int   TAGGER_SCALER_RATE_AVE;
int   TAGGER_SUM2;
int   TAGGER_HISTORY_P2S;
int   TAGGER_HISTORY_LIVE;
int   TAGGER_HISTORY;
int   TAGGER_HISTORY_E100;
int   TAGGER_HISTORY_SINGLE[MAX_TAGGER];
int   TAGGER_HISTORY_bins;
int   TAGGER_CAMAC;
int   TAGGER_CAMAC_SUM;
int   TAGGER_LEAD_GLAS;
int   TAGGER_CH_LEAD_GLAS;
int   TAGGER_TDC_ONLY;
int   TAGGER_TDC_ONLY_TIME;
int   TAGGER_LATCH_ONLY;
int   TAGGER_TDC_LATCH;

/*int   MICRO_MULT;
int   MICRO_CHANNEL;
int   MICRO_TIME;
int   MICRO_TIME_SINGLE[MAX_MICROSCOPE];
int   MICRO_ENERGY_SINGLE[MAX_MICROSCOPE];
int   MICRO_VS_TAGGER;
int   MICRO_SCALER;
int   MICRO_SCALER_SUM;
*/

int   SANE_CELL_SCALER;
int   SANE_CELL_SCALER_SUM;
int   SANE_CELL_SCALER_RATE;
int   SANE_CELL_SCALER_RATE_AVE;
int   SANE_VETO_SCALER;
int   SANE_VETO_SCALER_SUM;
int   SANE_VETO_SCALER_RATE;
int   SANE_VETO_SCALER_RATE_AVE;
int   SANE_COL_SCALER;
int   SANE_COL_SCALER_SUM;
int   SANE_COL_SCALER_RATE;
int   SANE_COL_SCALER_RATE_AVE;
int   SANE_TRIG_SCALER;
int   SANE_TRIG_SCALER_SUM;
int   SANE_TRIG_SCALER_RATE;
int   SANE_TRIG_SCALER_RATE_AVE;

int   TRIGGER_SCALER_RAW;
int   TRIGGER_SCALER_RAW_SUM;
int   TRIGGER_SCALER_INH;
int   TRIGGER_SCALER_INH_SUM;
int   TRIGGER_SCALER_RED;
int   TRIGGER_SCALER_RED_SUM;
int   TRIGGER_STAT;

/*
int   LASER_WIDE[MAX_LASER_WIDE];
int   LASER_NARROW[MAX_LASER_NARROW];
int   LASER_DIODE[MAX_DIODE];

int   GLOBAL_SCALER;
int   GLOBAL_SCALER_SUM;
*/

int book_standard_spectra()
{

	int i;
	char help[100];  
	histo* h_entry;

	assign_array("pileup", PILEUP_TDC_SPEC);
	PILEUP_25 = assign_id("pileup_25");
	L_HIT_PATTERN = assign_id("l.hit.pattern");
  
/*-----------------------------------------------------------------------
 * 
 * TAPS Blocks A-F histograms
 * 
 */

// Wide gate
	assign_array("a.wide",T_WIDE[1]);
	assign_array("b.wide",T_WIDE[2]);
	assign_array("c.wide",T_WIDE[3]);
	assign_array("d.wide",T_WIDE[4]);
	assign_array("e.wide",T_WIDE[5]);
	assign_array("f.wide",T_WIDE[6]);
  
/*
// Narrow gate
	assign_array("a.narrow",T_NARROW[1]);
	assign_array("b.narrow",T_NARROW[2]);
	assign_array("c.narrow",T_NARROW[3]);
	assign_array("d.narrow",T_NARROW[4]);
	assign_array("e.narrow",T_NARROW[5]);
	assign_array("f.narrow",T_NARROW[6]);
  
// Pulse Shape Analysis
	assign_array("a.psa",T_PSA[1]);
	assign_array("b.psa",T_PSA[2]);
	assign_array("c.psa",T_PSA[3]);
	assign_array("d.psa",T_PSA[4]);
	assign_array("e.psa",T_PSA[5]);
	assign_array("f.psa",T_PSA[6]);

	T_PSA_2D_ALL = assign_id("taps.psa.2d");

	for(i=1;i<BAFNUMBER;i++) {
	    sprintf(help,"a.psa.2d_%d",i);
	    T_PSA_2D[1][i] = assign_id(help);
	    sprintf(help,"b.psa.2d_%d",i);
	    T_PSA_2D[2][i] = assign_id(help);
	    sprintf(help,"c.psa.2d_%d",i);
	    T_PSA_2D[3][i] = assign_id(help);
	    sprintf(help,"d.psa.2d_%d",i);
	    T_PSA_2D[4][i] = assign_id(help);
	    sprintf(help,"e.psa.2d_%d",i);
	    T_PSA_2D[5][i] = assign_id(help);
	    sprintf(help,"f.psa.2d_%d",i);
	    T_PSA_2D[6][i] = assign_id(help);
	}
*/
  
// Time
	assign_array("a.time",T_TIME[1]);
	assign_array("b.time",T_TIME[2]);
	assign_array("c.time",T_TIME[3]);
	assign_array("d.time",T_TIME[4]);
	assign_array("e.time",T_TIME[5]);
	assign_array("f.time",T_TIME[6]);
  
// Time vs Energy
	T_TIME_2D_ALL = assign_id("taps.time.2d");

	for(i=1;i<BAFNUMBER;i++) {
	    sprintf(help,"a.time.2d_%d",i);
	    T_TIME_2D[1][i] = assign_id(help);
	    sprintf(help,"b.time.2d_%d",i);
	    T_TIME_2D[2][i] = assign_id(help);
	    sprintf(help,"c.time.2d_%d",i);
	    T_TIME_2D[3][i] = assign_id(help);
	    sprintf(help,"d.time.2d_%d",i);
	    T_TIME_2D[4][i] = assign_id(help);
	    sprintf(help,"e.time.2d_%d",i);
	    T_TIME_2D[5][i] = assign_id(help);
	    sprintf(help,"f.time.2d_%d",i);
	    T_TIME_2D[6][i] = assign_id(help);
}
  

// Vetos
	assign_array("b.veto",T_VETO_B);
	assign_array("e.veto",T_VETO_E);

// Pattern
	T_PATTERN_BAF[1]     = assign_id("a.pattern.cfd");
	T_PATTERN_BAF[2]     = assign_id("b.pattern.cfd");
	T_PATTERN_BAF[3]     = assign_id("c.pattern.cfd");
	T_PATTERN_BAF[4]     = assign_id("d.pattern.cfd");
	T_PATTERN_BAF[5]     = assign_id("e.pattern.cfd");
	T_PATTERN_BAF[6]     = assign_id("f.pattern.cfd");
	T_PATTERN_BAF_ALL    = assign_id("taps.pattern.cfd");

	T_PATTERN_LED[1]     = assign_id("a.pattern.led");
	T_PATTERN_LED[2]     = assign_id("b.pattern.led");
	T_PATTERN_LED[3]     = assign_id("c.pattern.led");
	T_PATTERN_LED[4]     = assign_id("d.pattern.led");
	T_PATTERN_LED[5]     = assign_id("e.pattern.led");
	T_PATTERN_LED[6]     = assign_id("f.pattern.led");
	T_PATTERN_LED_ALL    = assign_id("taps.pattern.led");

	T_PATTERN_VETO[1]    = assign_id("a.pattern.veto");
	T_PATTERN_VETO[2]    = assign_id("b.pattern.veto");
	T_PATTERN_VETO[3]    = assign_id("c.pattern.veto");
	T_PATTERN_VETO[4]    = assign_id("d.pattern.veto");
	T_PATTERN_VETO[5]    = assign_id("e.pattern.veto");
	T_PATTERN_VETO[6]    = assign_id("f.pattern.veto");
	T_PATTERN_VETO_ALL   = assign_id("taps.pattern.veto");

	T_PATTERN_VETO_NOCFD = assign_id("taps.pattern.veto.nocfd");
	T_PATTERN_VETO_CFD   = assign_id("taps.pattern.veto.cfd");
	T_PATTERN_LED_NOCFD  = assign_id("taps.pattern.led.nocfd"); 
	T_PATTERN_CFD_NOWIDE = assign_id("taps.pattern.cfd.nowide");
//	T_PATTERN_CFD_NONARROW = assign_id("taps.pattern.cfd.nonarrow");
	T_PATTERN_CFD_NOTIME = assign_id("taps.pattern.cfd.notime");
	T_PATTERN_CFD_FULLWIDE = assign_id("taps.pattern.cfd.fullwide");
//	T_PATTERN_CFD_FULLNARROW = assign_id("taps.pattern.cfd.fullnarrow");
	T_PATTERN_CFD_FULLTIME = assign_id("taps.pattern.cfd.fulltime");

	T_BAF_THETA          = assign_id("taps.pattern.cfd.theta");
	T_BAF_PHI            = assign_id("taps.pattern.cfd.phi");
	T_BAF_GEOM           = assign_id("taps.pattern.cfd.geom");

	T_LED_GEOM           = assign_id("taps.pattern.led.geom");

	T_VETO_THETA         = assign_id("taps.pattern.veto.theta");
	T_VETO_PHI           = assign_id("taps.pattern.veto.phi");
	T_VETO_GEOM          = assign_id("taps.pattern.veto.geom");

	T_MULT[1]	     = assign_id("a.mult");
	T_MULT[2]	     = assign_id("b.mult");
	T_MULT[3]	     = assign_id("c.mult");
	T_MULT[4]	     = assign_id("d.mult");
	T_MULT[5]	     = assign_id("e.mult");
	T_MULT[6]	     = assign_id("f.mult");
	T_MULT_ALL     = assign_id("taps.mult");
  
/*-----------------------------------------------------------------------
 * 
 * Forward Wall histograms
 * 
 */

// Wide
	assign_array("fw.wide",F_WIDE);

/*
// Narrow
	assign_array("fw.narrow",F_NARROW);

// Pulse Shape Analysis
	assign_array("fw.psa",F_PSA);

	F_PSA_2D_ALL = assign_id("fw.psa.2d");

	for(i=1;i<MAX_FORWARD;i++) {
	    sprintf(help,"fw.psa.2d_%d",i);
	    F_PSA_2D[i] = assign_id(help);
	}
*/

// Time
	assign_array("fw.time",F_TIME);

// Time vs Energy
	F_TIME_2D_ALL = assign_id("fw.time.2d");

	for(i=1;i<MAX_FORWARD;i++) {
	    sprintf(help,"fw.time.2d_%d",i);
	    F_TIME_2D[i] = assign_id(help);
	}

// Pattern
	F_PATTERN_BAF = assign_id("fw.pattern.cfd");
	F_PATTERN_LED = assign_id("fw.pattern.led");
	F_PATTERN_VETO= assign_id("fw.pattern.veto");

	F_PATTERN_VETO_NOCFD = assign_id("fw.pattern.veto.nocfd");
	F_PATTERN_VETO_CFD   = assign_id("fw.pattern.veto.cfd");  
	F_PATTERN_LED_NOCFD  = assign_id("fw.pattern.led.nocfd");
	F_PATTERN_CFD_NOWIDE = assign_id("fw.pattern.cfd.nowide");
//	F_PATTERN_CFD_NONARROW = assign_id("fw.pattern.cfd.nonarrow");
	F_PATTERN_CFD_NOTIME = assign_id("fw.pattern.cfd.notime");
	F_PATTERN_CFD_FULLWIDE = assign_id("fw.pattern.cfd.fullwide");
//	F_PATTERN_CFD_FULLNARROW = assign_id("fw.pattern.cfd.fullnarrow");
	F_PATTERN_CFD_FULLTIME = assign_id("fw.pattern.cfd.fulltime");

	F_BAF_THETA   = assign_id("fw.pattern.cfd.theta");
	F_BAF_PHI     = assign_id("fw.pattern.cfd.phi");

	F_VETO_THETA         = assign_id("fw.pattern.veto.theta");
	F_VETO_PHI           = assign_id("fw.pattern.veto.phi");
	F_VETO_GEOM          = assign_id("fw.pattern.veto.geom");

	F_MULT	      = assign_id("fw.mult");

	TOTAL_MULT     = assign_id("total.mult");
  
	PATTERN_CFD		= assign_id("pattern.cfd");
	PATTERN_CFD_NOWIDE	= assign_id("pattern.cfd.nowide");
//	PATTERN_CFD_NONARROW	= assign_id("pattern.cfd.nonarrow");
	PATTERN_CFD_NOTIME	= assign_id("pattern.cfd.notime");
	PATTERN_CFD_FULLWIDE	= assign_id("pattern.cfd.fullwide");
//	PATTERN_CFD_FULLNARROW	= assign_id("pattern.cfd.fullnarrow");
	PATTERN_CFD_FULLTIME	= assign_id("pattern.cfd.fulltime");
	PATTERN_LED		= assign_id("pattern.led");
	PATTERN_LED_NOCFD	= assign_id("pattern.led.nocfd");
	PATTERN_VETO		= assign_id("pattern.veto");
	PATTERN_VETO_NOCFD	= assign_id("pattern.veto.nocfd");
	PATTERN_VETO_CFD	= assign_id("pattern.veto.cfd");

	LED_MULT			= assign_id("led.mult");
	VETO_MULT			= assign_id("veto.mult");
	LED_STAT			= assign_id("led.stat");
  
  
/*-----------------------------------------------------------------------
 * 
 * Tagger histograms
 * 
 */

	TAGGER_MULT        = assign_id("tagger.mult");
	TAGGER_ENERGY      = assign_id("tagger.energy");
	TAGGER_CHANNEL     = assign_id("tagger.channel");
	TAGGER_CHANNEL_T1  = assign_id("tagger.channel.t1");
	TAGGER_TIME        = assign_id("tagger.time");
	assign_array("tagger.time",TAGGER_TIME_SINGLE);
	TAGGER_CAMAC       = assign_id("tagger.camac");
	TAGGER_CAMAC_SUM   = assign_id("tagger.camac.sum");
	TAGGER_SCALER      = assign_id("tagger.scaler");
	TAGGER_SCALER_RATE = assign_id("tagger.scaler.rate");
	TAGGER_SCALER_SUM  = assign_id("tagger.scaler.sum");
	TAGGER_SCALER_RATE_AVE  = assign_id("tagger.scaler.rate.ave");
	TAGGER_SUM2        = assign_id("tagger.scaler.sum2");
	TAGGER_HISTORY_LIVE= assign_id("tagger.history.live");
	TAGGER_HISTORY_P2S = assign_id("tagger.history.p2s");
	TAGGER_HISTORY = assign_id("tagger.history");
	TAGGER_HISTORY_E100 = assign_id("tagger.history.e100");
	if ( (h_entry=get_h_entry("tagger.history")) != NULL ) {
	  TAGGER_HISTORY_bins = h_entry->nx;
	}
	assign_array("tagger.history",TAGGER_HISTORY_SINGLE);
	TAGGER_LEAD_GLAS   = assign_id("tagger.leadglas");
	TAGGER_CH_LEAD_GLAS   = assign_id("tagger.ch.leadglas");

	TAGGER_LATCH_ONLY       = assign_id("tagger.latch");
	TAGGER_TDC_ONLY         = assign_id("tagger.tdc");
	TAGGER_TDC_ONLY_TIME    = assign_id("tagger.tdc.time");
	TAGGER_TDC_LATCH        = assign_id("tagger.ok");
  
/*
// Microscope
	MICRO_MULT        = assign_id("micro.mult");
	MICRO_CHANNEL     = assign_id("micro.channel");
	MICRO_TIME        = assign_id("micro.time");
	assign_array("micro.time",MICRO_TIME_SINGLE);
	assign_array("micro.energy",MICRO_ENERGY_SINGLE);
	MICRO_VS_TAGGER   = assign_id("micro.tagger");
	MICRO_SCALER      = assign_id("micro.scaler");
	MICRO_SCALER_SUM  = assign_id("micro.scaler.sum");
*/

// SANE
	SANE_CELL_SCALER     = assign_id("sane.cell.scaler");
	SANE_CELL_SCALER_SUM = assign_id("sane.cell.scaler.sum");
	SANE_CELL_SCALER_RATE = assign_id("sane.cell.scaler.rate");
	SANE_CELL_SCALER_RATE_AVE = assign_id("sane.cell.scaler.rate.ave");
	SANE_VETO_SCALER     = assign_id("sane.veto.scaler");
	SANE_VETO_SCALER_SUM = assign_id("sane.veto.scaler.sum");
	SANE_VETO_SCALER_RATE = assign_id("sane.veto.scaler.rate");
	SANE_VETO_SCALER_RATE_AVE = assign_id("sane.veto.scaler.rate.ave");
	SANE_COL_SCALER      = assign_id("sane.col.scaler");
	SANE_COL_SCALER_SUM  = assign_id("sane.col.scaler.sum");
	SANE_COL_SCALER_RATE  = assign_id("sane.col.scaler.rate");
	SANE_COL_SCALER_RATE_AVE  = assign_id("sane.col.scaler.rate.ave");
	SANE_TRIG_SCALER      = assign_id("sane.trig.scaler");
	SANE_TRIG_SCALER_SUM  = assign_id("sane.trig.scaler.sum");
	SANE_TRIG_SCALER_RATE  = assign_id("sane.trig.scaler.rate");
	SANE_TRIG_SCALER_RATE_AVE  = assign_id("sane.trig.scaler.rate.ave");

// Trigger
	TRIGGER_SCALER_RAW     = assign_id("trigger.scaler.raw");
	TRIGGER_SCALER_RAW_SUM = assign_id("trigger.scaler.raw.sum");
	TRIGGER_SCALER_INH     = assign_id("trigger.scaler.inhibit");
	TRIGGER_SCALER_INH_SUM = assign_id("trigger.scaler.inhibit.sum");
	TRIGGER_SCALER_RED     = assign_id("trigger.scaler.reduced");
	TRIGGER_SCALER_RED_SUM = assign_id("trigger.scaler.reduced.sum");
	TRIGGER_STAT	       = assign_id("trigger.stat");  

/*
// LASER / DIODE
	assign_array("laser.wide"  ,LASER_WIDE);
	assign_array("laser.narrow",LASER_NARROW);
	assign_array("laser.diode" ,LASER_DIODE);

	GLOBAL_SCALER	       = assign_id("global.scaler") ;
	GLOBAL_SCALER_SUM     = assign_id("global.scaler.sum") ;
*/
 
	return 0;
}

/************************************************************************
 * 
 * Fill all the data in the structures
 * 
 ************************************************************************/

void fill_standard_spectra()
{
	int i, j, current_time, led_mult, veto_mult, led_blmult[11];
	short bl, det, part, det_base;
	register float temp, num;

/*--------------------------------------------------------------------------
 * 
 * TAPS Blocks A-F
 * 
 */
	led_mult = 0;
	veto_mult = 0;
	memset( led_blmult, 0, sizeof( led_blmult));

	NHFILL1( TOTAL_MULT, (float) (fdet_hit_pointer + taps_hit_pointer), 0. , 1.);

	NHFILL1( T_MULT_ALL, (float) taps_hit_pointer, 0. , 1.);

	for ( i = 1; i < BLOCKNUMBER; i++) {
		NHFILL1( T_MULT[i], (float) taps[i].hits, 0. , 1.);
	}

//	printf( "TAPS Blocks A-F\n");
	// TAPS Blocks
	//	First fill spectra that don't need LED or VETO checks
	for ( i = 1; i <= taps_hit_pointer; i++) {
		bl  = taps_hits[i].block;
		det = taps_hits[i].baf;
		num = (float) (bl - 1)*64 + det;
//		printf( "   %d  %2d  %f  %f  %f  %f\n", bl, det, taps[bl].e_long[det], taps[bl].e_short[det], taps[bl].time[det],
//				taps[bl].psa[det]);
		if (control_flags & FILL_BAFS_ON_CFD) {
			NHFILL1( T_WIDE[bl][det], taps[bl].e_long[det], 0., 1.);
//			NHFILL1( T_NARROW[bl][det], taps[bl].e_short[det], 0., 1.);
			NHFILL1( T_TIME[bl][det], taps[bl].time[det], 0., 1.);
//			NHFILL1( T_PSA[bl][det], taps[bl].psa[det], 0., 1.);
//			NHFILL2( T_PSA_2D[bl][det], taps[bl].e_long[det], taps[bl].e_short[det], 1.);
//			NHFILL2( T_PSA_2D_ALL, taps[bl].e_long[det], taps[bl].e_short[det], 1.);
			NHFILL2( T_TIME_2D[bl][det], taps[bl].time[det], taps[bl].e_long[det], 1.);
			NHFILL1( T_PATTERN_BAF[bl], (float) det, 0., 1.);
			NHFILL1( T_PATTERN_BAF_ALL, num, 0., 1.);
			NHFILL1( PATTERN_CFD, num, 0., 1.);
			NHFILL1( T_BAF_THETA, taps_geom[bl].mtheta[det], 0., 1.);        
			NHFILL1( T_BAF_PHI, taps_geom[bl].mphi[det], 0., 1.);        
			NHFILL2( T_BAF_GEOM, -taps_geom[bl].mtheta_taps[det], taps_geom[bl].mphi_taps[det], 1.);
//			if (taps[bl].e_short[det] < 0.5) {
//				NHFILL1( T_PATTERN_CFD_NONARROW, num, 0., 1.);
//				NHFILL1( PATTERN_CFD_NONARROW, num, 0., 1.);
//			}
			if (taps[bl].e_long[det] < 0.5) {
				NHFILL1( T_PATTERN_CFD_NOWIDE, num, 0., 1.);
				NHFILL1( PATTERN_CFD_NOWIDE, num, 0., 1.);
			}
			if (taps[bl].time[det] < 0.5) {
				NHFILL1( T_PATTERN_CFD_NOTIME, num, 0., 1.);
				NHFILL1( PATTERN_CFD_NOTIME, num, 0., 1.);
			}
//			if (taps[bl].e_short[det] > 4094.5) {
//				NHFILL1( T_PATTERN_CFD_FULLNARROW, num, 0., 1.);
//				NHFILL1( PATTERN_CFD_FULLNARROW, num, 0., 1.);
//			}
			if (taps[bl].e_long[det] > 4094.5) {
				NHFILL1( T_PATTERN_CFD_FULLWIDE, num, 0., 1.);
				NHFILL1( PATTERN_CFD_FULLWIDE, num, 0., 1.);
			}
			if (taps[bl].time[det] > 4094.5) {
				NHFILL1( T_PATTERN_CFD_FULLTIME, num, 0., 1.);
				NHFILL1( PATTERN_CFD_FULLTIME, num, 0., 1.);
			}
		}
	}

	//	Now fill spectra that _DO_ need LED or VETO checks
	for ( bl = 1; bl < BLOCKNUMBER; bl++) {
		for ( part = 0; part <= 1; part++) {
			det_base = part*32;
			for ( i = 0; i < 32; i++) {
				det = det_base + i + 1;
				if ( taps[bl].veto_pattern[part] & (1<<i) ) {	// check VETO
					veto_mult++;
					taps[bl].flags[det] |= VETO;
					if (!(taps[bl].flags[det] & HIT)) {
						num = (float) (bl-1)*64+det;
						NHFILL1( PATTERN_VETO_NOCFD, num, 0., 1.);
					}
					NHFILL1( PATTERN_VETO,(float) (bl-1)*64+det, 0., 1.);
					NHFILL1( T_VETO_THETA, taps_geom[bl].mtheta[det], 0., 1.);        
					NHFILL1( T_VETO_PHI, taps_geom[bl].mphi[det], 0., 1.);        
					NHFILL2( T_VETO_GEOM, -taps_geom[bl].mtheta_taps[det], taps_geom[bl].mphi_taps[det], 1.);
					if (control_flags & FILL_BAFS_ON_VETO) {
						num = (float) (bl-1)*64+det;
						NHFILL1( T_WIDE[bl][det], taps[bl].e_long[det], 0., 1.);
//						NHFILL1( T_NARROW[bl][det], taps[bl].e_short[det], 0., 1.);
						NHFILL1( T_TIME[bl][det], taps[bl].time[det], 0., 1.);
//						NHFILL1( T_PSA[bl][det], taps[bl].psa[det], 0., 1.);
//						NHFILL2( T_PSA_2D[bl][det], taps[bl].e_long[det], taps[bl].e_short[det], 1.);
//						NHFILL2( T_PSA_2D_ALL, taps[bl].e_long[det], taps[bl].e_short[det], 1.);
						NHFILL2( T_TIME_2D[bl][det], taps[bl].time[det], taps[bl].e_long[det], 1.);
					}
				}
				if ( taps[bl].led_pattern[part] & (1<<i) ) {		// check LED
					led_mult++;
					led_blmult[bl]++;
					taps[bl].flags[det] |= LED;
					if (!(taps[bl].flags[det] & HIT)) {
						num = (float) (bl-1)*64+det;
						NHFILL1( PATTERN_LED_NOCFD,num, 0., 1.);
					}
					NHFILL1( T_PATTERN_LED[bl],(float) det, 0., 1.);
					NHFILL1( T_PATTERN_LED_ALL,(float) (bl-1)*64+det, 0., 1.);

					NHFILL2( PILEUP_25, pileup_tdc[2], (float) (bl-1)*64+det, 1.);	// Su

					NHFILL1( PATTERN_LED,(float) (bl-1)*64+det, 0., 1.);
					NHFILL2( T_LED_GEOM, -taps_geom[bl].mtheta_taps[det], taps_geom[bl].mphi_taps[det] ,1.);        
					if (control_flags & FILL_BAFS_ON_LED) {
						num = (float) (bl-1)*64+det;
						NHFILL1( T_WIDE[bl][det], taps[bl].e_long[det], 0., 1.);
//						NHFILL1( T_NARROW[bl][det], taps[bl].e_short[det], 0., 1.);
						NHFILL1( T_TIME[bl][det], taps[bl].time[det], 0., 1.);
//						NHFILL1( T_PSA[bl][det], taps[bl].psa[det], 0., 1.);
//						NHFILL2( T_PSA_2D[bl][det], taps[bl].e_long[det], taps[bl].e_short[det], 1.);
//						NHFILL2( T_PSA_2D_ALL, taps[bl].e_long[det], taps[bl].e_short[det], 1.);
						NHFILL2( T_TIME_2D[bl][det], taps[bl].time[det], taps[bl].e_long[det], 1.);
					}
				}
			} // end half
		} // end part
	} // end block

/*--------------------------------------------------------------------------
 * 
 * Forward Wall
 * 
 */

//	printf( "FW Block\n");

	NHFILL1( F_MULT, (float) fdet_hit_pointer, 0. , 1.);

	//  Fill spectra that don't need LED or VETO checks
	for ( i = 1; i <= fdet_hit_pointer; i++) {
		det = fdet_hits[i].module;

//		if ( fdet.time[det] != -10000) printf( "   %3d  %f  %f  %f  %f\n", det, fdet.e_long[det], fdet.e_short[det],
//				fdet.time[det], fdet.psa[det]);

		if (control_flags & FILL_BAFS_ON_CFD) {
			NHFILL1( F_WIDE[det], fdet.e_long[det], 0., 1.);
//			NHFILL1( F_NARROW[det], fdet.e_short[det], 0., 1.);
			NHFILL1( F_TIME[det], fdet.time[det], 0., 1.);
//			NHFILL1( F_PSA[det], fdet.psa[det], 0., 1.);
//			NHFILL2( F_PSA_2D[det], fdet.e_long[det], fdet.e_short[det], 1.);
//			NHFILL2( F_PSA_2D_ALL, fdet.e_long[det], fdet.e_short[det], 1.);
			NHFILL2( F_TIME_2D[det], fdet.time[det], fdet.e_long[det], 1.);
		}    
		NHFILL1( F_PATTERN_BAF, (float) det, 0., 1.);
		NHFILL1( PATTERN_CFD, (float) (det+384), 0., 1.);
		NHFILL1( F_BAF_THETA, fdet_geom.mtheta[det], 0., 1.);        
		NHFILL1( F_BAF_PHI, fdet_geom.mphi[det], 0., 1.);        
		NHFILL2( T_BAF_GEOM, -fdet_geom.mtheta_taps[det], fdet_geom.mphi_taps[det], 1.);        
//		if (fdet.e_short[det] < 0.5) {
//			NHFILL1( F_PATTERN_CFD_NONARROW, (float) det, 0., 1.);
//			NHFILL1( PATTERN_CFD_NONARROW, (float) (det+384), 0., 1.);
//		}
		if (fdet.e_long[det] < 0.5) {
			NHFILL1( F_PATTERN_CFD_NOWIDE, (float) det, 0., 1.);
			NHFILL1( PATTERN_CFD_NOWIDE, (float) (det+384), 0., 1.);
		}
		if (fdet.time[det] < 0.5) {
			NHFILL1( F_PATTERN_CFD_NOTIME, (float) det, 0., 1.);
			NHFILL1( PATTERN_CFD_NOTIME, (float) (det+384), 0., 1.);
		}
//		if (fdet.e_short[det] > 4094.5) {
//			NHFILL1( F_PATTERN_CFD_FULLNARROW, (float) det, 0., 1.);
//			NHFILL1( PATTERN_CFD_FULLNARROW, (float) (det+384), 0., 1.);
//		}
		if (fdet.e_long[det] > 4094.5) {
			NHFILL1( F_PATTERN_CFD_FULLWIDE, (float) det, 0., 1.);
			NHFILL1( PATTERN_CFD_FULLWIDE, (float) (det+384), 0., 1.);
		}
		if (fdet.time[det] > 4094.5) {
			NHFILL1( F_PATTERN_CFD_FULLTIME, (float)det, 0., 1.);
			NHFILL1( PATTERN_CFD_FULLTIME, (float) (det+384), 0., 1.);
		}
	}

	//	Now fill spectra that _DO_ need LED or VETO checks
	for ( part = 0; part <= 4; part++) {
		int maxdet;

		if ( part == 4) {
			det = 16;
			det_base = 126;
		}  
		else if (( part == 1) || ( part == 2)) {
			maxdet = 31;
			det_base = 32 + (part - 1)*31;
		}  
		else if ( part == 0) {
			maxdet = 32;
			det_base = 0;
		}
		else {
			maxdet = 32;
			det_base = 94;
		}  
		for ( i = 0; i < maxdet; i++) {
			det = det_base + i + 1;  
			if ( fdet.veto_pattern[part] & (1<<i) ) {			// check VETO
				veto_mult++;
				fdet.flags[det] |= VETO;
				if (!(fdet.flags[det] & HIT)) {
					NHFILL1( F_PATTERN_VETO_NOCFD,(float)det, 0., 1.);
					NHFILL1( PATTERN_VETO_NOCFD,(float)(det+384), 0., 1.);
				}
				else {
					NHFILL1( F_PATTERN_VETO_CFD,(float)det, 0., 1.);
					NHFILL1( PATTERN_VETO_CFD,(float)(det+384), 0., 1.);
				}
				if (control_flags & FILL_BAFS_ON_VETO) {
					NHFILL1( F_WIDE[det], fdet.e_long[det], 0., 1.);
//					NHFILL1( F_NARROW[det], fdet.e_short[det],0., 1.);
					NHFILL1( F_TIME[det], fdet.time[det], 0., 1.);
//					NHFILL1( F_PSA[det], fdet.psa[det], 0., 1.);
//					NHFILL2( F_PSA_2D[det], fdet.e_long[det], fdet.e_short[det], 1.);
//					NHFILL2( F_PSA_2D_ALL, fdet.e_long[det], fdet.e_short[det], 1.);
					NHFILL2( F_TIME_2D[det], fdet.time[det], fdet.e_long[det], 1.);
				}
				NHFILL1( F_PATTERN_VETO, (float) det, 0., 1.);
				NHFILL1( PATTERN_VETO, (float) (det+384), 0., 1.);
				NHFILL1( F_VETO_THETA, fdet_geom.mtheta[det], 0., 1.);
				NHFILL1( F_VETO_PHI, fdet_geom.mphi[det], 0., 1.);
				NHFILL2( F_VETO_GEOM, -fdet_geom.mtheta_taps[det], fdet_geom.mphi_taps[det] ,1.);
    				NHFILL2( T_VETO_GEOM, -fdet_geom.mtheta_taps[det], fdet_geom.mphi_taps[det] ,1.);        
			}
			if ( fdet.led_pattern[part] & (1<<i) ) {			// check LED
				led_mult++;
				if ( part < 5) led_blmult[FDET_BLOCK_NUMBER+part]++;
				fdet.flags[det] |= LED;
				NHFILL1( F_PATTERN_LED, (float) det, 0., 1.);
				NHFILL1( PATTERN_LED, (float) (det+384), 0., 1.);

				NHFILL2( PILEUP_25, pileup_tdc[2], (float) (384+det), 1.);	// Su

				NHFILL2( T_LED_GEOM, -fdet_geom.mtheta_taps[det], fdet_geom.mphi_taps[det], 1.);
				if (!(fdet.flags[det] & HIT)) {
					NHFILL1( F_PATTERN_LED_NOCFD, (float) det, 0., 1.);
					NHFILL1( PATTERN_LED_NOCFD, (float)(det+384), 0., 1.);
				}
				if (control_flags & FILL_BAFS_ON_LED) {
					NHFILL1( F_WIDE[det], fdet.e_long[det], 0., 1.);
//					NHFILL1( F_NARROW[det], fdet.e_short[det], 0., 1.);
					NHFILL1( F_TIME[det], fdet.time[det], 0., 1.);
//					NHFILL1( F_PSA[det], fdet.psa[det], 0., 1.);
//					NHFILL2( F_PSA_2D[det], fdet.e_long[det], fdet.e_short[det], 1.);
//					NHFILL2( F_PSA_2D_ALL, fdet.e_long[det], fdet.e_short[det], 1.);
					NHFILL2( F_TIME_2D[det], fdet.time[det], fdet.e_long[det], 1.);
				}
			}
		}
	}

	NHFILL1( LED_MULT, (float) led_mult, 0. , 1.);
	NHFILL1( VETO_MULT, (float) veto_mult, 0. , 1.);

	for ( i = 1; i < 11; i++) {   
		NHFILL2( LED_STAT, (float) i, (float) led_blmult[i], 1.);
	}
  
/*--------------------------------------------------------------------------
 * 
 * Tagger
 * 
 */

	NHFILL1( TAGGER_MULT,(float) tagger_pointer, 0., 1.);
	NHFILL1( TAGGER_LEAD_GLAS, lead_glas, 0., 1.);

	for ( i = 1; i <= tagger_pointer; i++) {
		det = tagger_hit[i];
		NHFILL1( TAGGER_ENERGY, tagger[det].energy,  0., 1.);
		NHFILL1( TAGGER_CHANNEL, (float) det, 0., 1.);
		if ( tagger_pointer == 1) {
			NHFILL1( TAGGER_CHANNEL_T1, (float) det, 0., 1.);
		}
		NHFILL1( TAGGER_TIME, tagger[det].time, 0., 1.);
		NHFILL1( TAGGER_TIME_SINGLE[det], tagger[det].time, 0., 1.);
		NHFILL2( TAGGER_CH_LEAD_GLAS, lead_glas, (float) det, 1.);
	}

/*--------------------------------------------------------------------------
 * 
 * Tagger Latch
 * 
 */
  
{
	int bit;
      
	for ( i = 1; i < MAX_TAGGER; i++) {
		bit = tagger_latch[(i-1)/16] & (1<<((i-1) % 16));
		if (bit && (tagger[i].type & TAGGER_TDC)) {
			NHFILL1(TAGGER_TDC_LATCH,(float) i, 0., 1.);
			tagger[i].type |= TAGGER_LATCH;
		}
		else if (bit) {
			NHFILL1(TAGGER_LATCH_ONLY,(float) i, 0., 1.);
			tagger[i].type |= TAGGER_LATCH;
		}
		else if (tagger[i].type & TAGGER_TDC) {
			NHFILL1(TAGGER_TDC_ONLY, (float) i, 0., 1.);
			NHFILL1(TAGGER_TDC_ONLY_TIME, tagger[i].time, 0., 1.);
		}
	}
}
  
/*--------------------------------------------------------------------------
 * 
 * Microscope
 * 

	NHFILL1(MICRO_MULT,(float) micro_pointer, 0., 1.);

	for (i=1;i<=micro_pointer;i++) {
		det = micro_hit[i];
		NHFILL1(MICRO_CHANNEL,(float) det, 0., 1.);
		NHFILL1(MICRO_TIME,micro[det].time, 0., 1.);
		NHFILL1(MICRO_TIME_SINGLE[det],micro[det].time, 0., 1.);
		NHFILL1(MICRO_ENERGY_SINGLE[det],micro[det].energy, 0., 1.);
		for(j=1;j<=tagger_pointer;j++) {
			NHFILL2(MICRO_VS_TAGGER,(float) det, (float) tagger_hit[j], 1.);
		}
	}
  
	if (trigger_pattern & (1<<7)) {
		for(i=1;i<MAX_LASER_WIDE;i++) {
			NHFILL1(LASER_WIDE[i], laser.wide[i], 0. ,1.);
		}
		for(i=1;i<MAX_LASER_NARROW;i++) {
			NHFILL1(LASER_NARROW[i], laser.narrow[i], 0. ,1.);
		}
		for(i=1;i<MAX_DIODE;i++) {
			NHFILL1(LASER_DIODE[i], laser.diode[i], 0. ,1.);
		}
	}
 */

/*--------------------------- PILEUP TDC -----------------------------*/  

	for ( i = 1; i <= 8; i++) {
		NHFILL1(PILEUP_TDC_SPEC[i], pileup_tdc[i], 0., 1.);
	}

//	NHFILL1(PILEUP_25, pileup_tdc[2] - pileup_tdc[5], 0., 1.); 	Su
  
	for ( i = 0; i <= 15; i++) {
		if (L_hit_pattern & (1<<i)) {    
			NHFILL1(L_HIT_PATTERN, (float) i+1, 0., 1.);  
		}
	}  
  
	return;
}

/**************************************************************************
 * 
 * Fill Scaler Spectra
 * 
 */

void fill_scaler_spectra()
{

	static long scalersum[MAX_TAGGER], scalersum2[MAX_TAGGER];
	static long sane_scalersum[MAX_SANE_SCALER];
	int i, j, k, l, m;
  
/*--------------------------------------------------------------------------
 * 
 * Triggerscaler
 * 
 */

	if ( event_status & ESR_TRIGGER_SCALER_EVENT ) {
		if (TRIGGER_SCALER_RAW) HRESET(TRIGGER_SCALER_RAW," ");
		if (TRIGGER_SCALER_INH) HRESET(TRIGGER_SCALER_INH," ");
		if (TRIGGER_SCALER_RED) HRESET(TRIGGER_SCALER_RED," ");
		if (num_trigger_scaler >= 24) {
			for (i=1;i<=24;i++) {
				NHFILL1(TRIGGER_SCALER_RAW    ,(float) i, 0.,(float) trigger_scaler[i]);
				NHFILL1(TRIGGER_SCALER_RAW_SUM,(float) i, 0.,(float) trigger_scaler[i]);
			}
		}
		if (num_trigger_scaler >= 48) {
			for (i=1;i<=24;i++) {
				NHFILL1(TRIGGER_SCALER_INH    ,(float) i, 0.,(float) trigger_scaler[i+24]);
				NHFILL1(TRIGGER_SCALER_INH_SUM,(float) i, 0.,(float) trigger_scaler[i+24]);
			}
		}
		if (num_trigger_scaler >= 72) {
			for (i=1;i<=24;i++) {
				NHFILL1(TRIGGER_SCALER_RED    ,(float) i, 0.,(float) trigger_scaler[i+48]);
				NHFILL1(TRIGGER_SCALER_RED_SUM,(float) i, 0.,(float) trigger_scaler[i+48]);
			}
		}
	}

	if (trigger_pattern&(1<<6)) {
		NHFILL2(TRIGGER_STAT, 1., 1., 1.);
		if (trigger_pattern&(1<<26)) {
			NHFILL2(TRIGGER_STAT, 2., 2., 1.);
			if (trigger_pattern&(1<<27)) NHFILL2(TRIGGER_STAT, 3., 2., 1.); 
			if (trigger_pattern&(1<<24)) NHFILL2(TRIGGER_STAT, 4., 2., 1.); 
			if (trigger_pattern&(1<<25)) NHFILL2(TRIGGER_STAT, 5., 2., 1.); 
		}
		if (trigger_pattern&(1<<27)) {
			NHFILL2(TRIGGER_STAT, 3., 3., 1.);
				if (trigger_pattern&(1<<26)) NHFILL2(TRIGGER_STAT, 2., 3., 1.); 
				if (trigger_pattern&(1<<24)) NHFILL2(TRIGGER_STAT, 4., 3., 1.); 
				if (trigger_pattern&(1<<25)) NHFILL2(TRIGGER_STAT, 5., 3., 1.); 
		}
		if (trigger_pattern&(1<<24)) {
			NHFILL2(TRIGGER_STAT, 4., 4., 1.);
			if (trigger_pattern&(1<<26)) NHFILL2(TRIGGER_STAT, 2., 4., 1.); 
			if (trigger_pattern&(1<<27)) NHFILL2(TRIGGER_STAT, 3., 4., 1.); 
			if (trigger_pattern&(1<<25)) NHFILL2(TRIGGER_STAT, 5., 4., 1.); 
		}
		if (trigger_pattern&(1<<25)) {
			NHFILL2(TRIGGER_STAT, 5., 5., 1.);
			if (trigger_pattern&(1<<26)) NHFILL2(TRIGGER_STAT, 2., 5., 1.); 
			if (trigger_pattern&(1<<27)) NHFILL2(TRIGGER_STAT, 3., 5., 1.); 
			if (trigger_pattern&(1<<24)) NHFILL2(TRIGGER_STAT, 4., 5., 1.); 
		}
		if (trigger_pattern&(1<<16)) NHFILL2(TRIGGER_STAT, 1., 6., 1.);
	}
	if (trigger_pattern&(1<<8)) {
		NHFILL2(TRIGGER_STAT, 6., 6., 1.);
	}
  
/*--------------------------------------------------------------------------
 * 
 * Taggerscaler
 * 
 */

	if ( event_status & ESR_SCALER_EVENT ) {

		float scaler_sum, old_entry;
		int next_position;
		float p2tos, livetime;
		float realtime, rem;
		static float totaltime;
		double hours;
		extern short do_pi0_dump;
		extern int n_scaler_events;
		extern float scaler_ext[353];
		extern float realtime_ext;

		n_scaler_events++;
//		if ( n_scaler_events%50 == 0) {
		if ( n_scaler_events%200 == 0) {
			printf( "<I> Number of Scaler Events = %6d\n", n_scaler_events);
			do_pi0_dump = 1;
		}

		scaler_sum = 0;
		if (TAGGER_SCALER) HRESET(TAGGER_SCALER," ");
		if (TAGGER_CAMAC)  HRESET(TAGGER_CAMAC," ");
		if (TAGGER_SUM2)   HRESET(TAGGER_SUM2," ");
    
		current_position++;
		if (current_position > TAGGER_HISTORY_bins) current_position = 1;
		next_position = current_position + 1;
		if (next_position > TAGGER_HISTORY_bins) next_position = 1;
  
		for (i=1;i<MAX_TAGGER;i++) {
			scaler_sum += (float) tagger[i].scaler;
			if (TAGGER_HISTORY_SINGLE[i]) old_entry = HX(TAGGER_HISTORY_SINGLE[i], (float) next_position);
			NHFILL1(TAGGER_HISTORY_SINGLE[i],(float)next_position,0., (-old_entry) );
			NHFILL1(TAGGER_HISTORY_SINGLE[i],(float)current_position,0., (float) tagger[i].scaler);
			NHFILL1(TAGGER_SCALER    ,(float) i,0.,(float) tagger[i].scaler);
			NHFILL1(TAGGER_SCALER_SUM,(float) i,0.,(float) tagger[i].scaler);

			scalersum[i] += tagger[i].scaler;
			NHFILL1(TAGGER_SUM2, (float) i, 0. ,(float) scalersum[i]);

			scaler_ext[i] += (float) tagger[i].scaler;
		}

		if (TAGGER_HISTORY) old_entry = HX(TAGGER_HISTORY, (float) next_position);

/*
		NHFILL1(TAGGER_HISTORY,(float) current_position, 0., ((float) scaler_sum)/(MAX_TAGGER-1));
		NHFILL1(TAGGER_HISTORY,(float) next_position,0., (-old_entry) );
*/
    
		NHFILL1(TAGGER_HISTORY, (float) n_scaler_events, 0., ((float) scaler_sum)/(MAX_TAGGER-1));

		if (TAGGER_HISTORY_P2S) old_entry = HX(TAGGER_HISTORY_P2S, (float) next_position);

		if (scaler_sum != 0) p2tos = (100.*10000.*trigger_scaler[10])/((float) scaler_sum);
		else p2tos = 0;
  
/*
		NHFILL1(TAGGER_HISTORY_P2S,(float)current_position,0., p2tos);
		NHFILL1(TAGGER_HISTORY_P2S,(float) next_position,0., (-old_entry) );
*/

		NHFILL1(TAGGER_HISTORY_P2S, (float) n_scaler_events, 0., p2tos);

		if (TAGGER_HISTORY_LIVE) old_entry = HX(TAGGER_HISTORY_LIVE, (float) next_position);

		if (camac_scaler[11] != 0) livetime = (100.*camac_scaler[12])/((float) camac_scaler[11]);
		else livetime = 0;
  
/*
		NHFILL1(TAGGER_HISTORY_LIVE,(float)current_position,0., livetime);
		NHFILL1(TAGGER_HISTORY_LIVE,(float) next_position,0., (-old_entry) );
*/
    
		NHFILL1(TAGGER_HISTORY_LIVE, (float) n_scaler_events, 0., livetime);

		for (i=1;i<MAX_CAMAC_SCALER;i++) {
			NHFILL1(TAGGER_CAMAC    ,(float) i,0., (float) camac_scaler[i]);
			NHFILL1(TAGGER_CAMAC_SUM,(float) i,0., (float) camac_scaler[i]);
		}

		realtime = camac_scaler[11]/1e6;
		realtime_ext += camac_scaler[11]/1e6;
		totaltime += realtime;

		if ( TAGGER_SCALER_RATE) HRESET( TAGGER_SCALER_RATE, " ");
		if ( TAGGER_SCALER_RATE_AVE) HRESET( TAGGER_SCALER_RATE_AVE, " ");
		for ( i = 1; i < MAX_TAGGER; i++) {
			scalersum2[i] += tagger[i].scaler;
			NHFILL1( TAGGER_SCALER_RATE, (float) i, 0., (float) tagger[i].scaler/realtime/1000.);
			NHFILL1( TAGGER_SCALER_RATE_AVE, (float) i, 0., (float) scalersum2[i]/totaltime/1000.);
		}

		NHFILL1( TAGGER_HISTORY_E100, (float) n_scaler_events, 0., tagger[100].scaler/realtime/1000.);

/*		if (GLOBAL_SCALER) HRESET(GLOBAL_SCALER," ");
		for (i=0;i<MAX_TOT_SCALER;i++) {
			NHFILL1(GLOBAL_SCALER    , (float) i, 0., (float) global_scaler[i]);
			NHFILL1(GLOBAL_SCALER_SUM, (float) i, 0., (float) global_scaler[i]);
		}        
    
		if (MICRO_SCALER) HRESET(MICRO_SCALER," ");
		for (i=1;i<MAX_MICROSCOPE;i++) {
			NHFILL1(MICRO_SCALER    ,(float) i,0.,(float) micro[i].scaler);
			NHFILL1(MICRO_SCALER_SUM,(float) i,0.,(float) micro[i].scaler);
		}
*/

		j = 0;
		k = 0;
		l = 0;
		m = 0;
		if ( SANE_CELL_SCALER) HRESET( SANE_CELL_SCALER, " ");
		if ( SANE_CELL_SCALER_RATE) HRESET( SANE_CELL_SCALER_RATE, " ");
		if ( SANE_CELL_SCALER_RATE_AVE) HRESET( SANE_CELL_SCALER_RATE_AVE, " ");
		if ( SANE_VETO_SCALER) HRESET( SANE_VETO_SCALER, " ");
		if ( SANE_VETO_SCALER_RATE) HRESET( SANE_VETO_SCALER_RATE, " ");
		if ( SANE_VETO_SCALER_RATE_AVE) HRESET( SANE_VETO_SCALER_RATE_AVE, " ");
		if ( SANE_COL_SCALER) HRESET( SANE_COL_SCALER, " ");
		if ( SANE_COL_SCALER_RATE) HRESET( SANE_COL_SCALER_RATE, " ");
		if ( SANE_COL_SCALER_RATE_AVE) HRESET( SANE_COL_SCALER_RATE_AVE, " ");
		if ( SANE_TRIG_SCALER) HRESET( SANE_TRIG_SCALER, " ");
		if ( SANE_TRIG_SCALER_RATE) HRESET( SANE_TRIG_SCALER_RATE, " ");
		if ( SANE_TRIG_SCALER_RATE_AVE) HRESET( SANE_TRIG_SCALER_RATE_AVE, " ");
		for ( i = 1; i < MAX_SANE_SCALER; i++) {
			sane_scalersum[i] += sane.scaler[i];
			if ( i < 92) {
				if ( ( i != 7) && ( i != 8) && ( i != 79) && ( i != 80)) {
					++j;
					NHFILL1( SANE_CELL_SCALER    , (float) j, 0., (float) sane.scaler[i]);
					NHFILL1( SANE_CELL_SCALER_SUM, (float) j, 0., (float) sane.scaler[i]);
					NHFILL1( SANE_CELL_SCALER_RATE, (float) j, 0., (float) sane.scaler[i]/realtime/1000);
					NHFILL1( SANE_CELL_SCALER_RATE_AVE, (float) j, 0., (float) sane_scalersum[i]/totaltime/1000);
				}
			}
			else if ( ( i > 96) && ( i < 109)) {
				++k;
				NHFILL1( SANE_VETO_SCALER, (float) k, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_VETO_SCALER_SUM, (float) k, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_VETO_SCALER_RATE, (float) k, 0., (float) sane.scaler[i]/realtime/1000);
				NHFILL1( SANE_VETO_SCALER_RATE_AVE, (float) k, 0., (float) sane_scalersum[i]/totaltime/1000);
			}
			else if ( ( i > 128) && ( i < 141)) {
				++l;
				NHFILL1( SANE_COL_SCALER, (float) l, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_COL_SCALER_SUM, (float) l, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_COL_SCALER_RATE, (float) l, 0., (float) sane.scaler[i]/realtime);
				NHFILL1( SANE_COL_SCALER_RATE_AVE, (float) l, 0., (float) sane_scalersum[i]/totaltime);
			}
			else if ( i > 140) {
				++m;
				NHFILL1( SANE_TRIG_SCALER, (float) m, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_TRIG_SCALER_SUM, (float) m, 0., (float) sane.scaler[i]);
				NHFILL1( SANE_TRIG_SCALER_RATE, (float) m, 0., (float) sane.scaler[i]/realtime/1000);
				NHFILL1( SANE_TRIG_SCALER_RATE_AVE, (float) m, 0., (float) sane_scalersum[i]/totaltime/1000);
			}
		}
	}  

	return;
}
