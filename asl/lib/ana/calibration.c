/* Calibration
 * 
 * 
 * Revision Date	Reason
 * ------------------------------------------------------------------------
 * 07.02.96	VH	Extracted from unpack_event.c
 * 28.02.01	DLH	Added SANE running gain calibration
 * 19.04.01	DLH	Added overflow stuff
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "asl.h"					// Library header file
#include "event.h"					// event description and geometry
#include "event_ext.h"				// extra event description

#if defined(OSF1) || defined(ULTRIX)	/* Platform dependencies */
# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

#define SECTION "CALIBRATION"			// Section name

/*****************************************************************************
 * 
 * File internal definitions
 * 
 */


void init_cal_values();
void read_cal_file( char* filename);
void read_cut_file( char* filename);
void update_cal_values( char* current_filename);

/*****************************************************************************/

extern int DEBUG;
extern int readout_mode;	       // from unpack_event.c

// Initialize "event.h"
	
float tagger_background_distance;
float tagger_background_factor;

// default calibration values
float cal_def_wide_gain, cal_def_wide_offset;
float cal_def_narrow_gain, cal_def_narrow_offset;
float cal_def_time_gain, cal_def_time_offset;
float cal_def_sane_wide_gain, cal_def_sane_wide_offset;
float cal_def_sane_narrow_gain, cal_def_sane_narrow_offset;
float cal_def_sane_time_gain, cal_def_sane_time_offset;
float SANE_LG_OVERFLOW[SANE_NCELL], SANE_SG_OVERFLOW[SANE_NCELL];
float SANE_TDC_OVERFLOW;

calibration_parameter cal;	  	// Calibration Structure
char runtime_calfile[100];

cutdef_proto cutdef[MAX_CUTDEF];

/*****************************************************************************
 * 
 * Structure of INITFILE:
 *
 * CAL_DEFAULT_TIME   1.0 0.0	gain offset
 * CAL_DEFAULT_WIDE   1.0 0.0
 * CAL_DEFAULT_NARROW 1.0 0.0
 * CAL_INIT			set these values
 *
 * CAL_RUNTIME  Filename	
 * 
 * CALIBRATE	Filename1	Calibration data
 * CALIBRATE	Filename2
 * ...
 *
 * TAGGER_BG    Distance Factor	
 * 
 * DO_CALIBRATE	  	0	switch 0/1
 *
 * HISTOGRAM ...		Standard entries
 * PICTURE   ...
 *
 *****************************************************************************/

int init_calibration()
{
	char initfile[100];
	char line[120];
	char key[40], arg[80], arg2[40];
	int bl, det, i;
	short tadc, lg_min, sg_min;

	FILE    *fp;

	// Status
	printf("<I> Init section: %s\n",SECTION);

	// Init several variables
	tagger_background_distance = 6.00;
	tagger_background_factor = 1.00;
	cal_def_wide_gain = 1.00;
	cal_def_wide_offset = 0.00;
	cal_def_narrow_gain = 1.00;
	cal_def_narrow_offset = 0.00;
	cal_def_time_gain = 1.00;
	cal_def_time_offset = 0.00;
	cal_def_sane_wide_gain = 1.00;
	cal_def_sane_wide_offset = 0.00;
	cal_def_sane_narrow_gain = 1.00;
	cal_def_sane_narrow_offset = 0.00;
	cal_def_sane_time_gain = 1.00;
	cal_def_sane_time_offset = 0.00;

	for ( i = 0; i < MAX_CUTDEF; i++) cutdef[i].dim = 0;
  
	init_cal_values();
  
	strcpy( runtime_calfile, "");

	// get initfile
	get_initfile( initfile, SECTION);
	if (initfile[0] == 0) {
		fprintf(stderr,"    No initfile for this section found\n");
		return -1;
	}

	// open initfile
	if ( ( fp = fopen( initfile, "r")) == NULL) {
		perror( "<E> Cannot open initfile");
		fprintf( stderr, "    File = %s\n", initfile);
		return -1;
	}

	// read initfile
	while ( fgets( line, sizeof( line), fp)) {
		if ( sscanf( line, "%s %s %s", key, arg, arg2) > 0) {
			if ( !strcmp( key, "HISTOGRAM")) read_h_entry( arg, fp);
			else if ( !strcmp( key, "PICTURE")) read_p_entry( arg, fp);
			else if ( !strcmp( key, "CAL_DEFAULT_WIDE")) {
				cal_def_wide_gain = atof( arg);
				cal_def_wide_offset = atof( arg2); 
			}
			else if ( !strcmp(key, "CAL_DEFAULT_NARROW")) {
				cal_def_narrow_gain = atof( arg);
				cal_def_narrow_offset = atof( arg2); 
			}
			else if ( !strcmp( key, "CAL_DEFAULT_TIME")) {
				cal_def_time_gain = atof( arg);
				cal_def_time_offset = atof( arg2); 
			}
			else if ( !strcmp( key, "CAL_DEFAULT_SANE_WIDE")) {
				cal_def_sane_wide_gain = atof( arg);
				cal_def_sane_wide_offset = atof( arg2); 
			}
			else if ( !strcmp( key, "CAL_DEFAULT_SANE_NARROW")) {
				cal_def_sane_narrow_gain = atof( arg);
				cal_def_sane_narrow_offset = atof( arg2); 
			}
			else if ( !strcmp( key, "CAL_DEFAULT_SANE_TIME")) {
				cal_def_sane_time_gain = atof( arg);
				cal_def_sane_time_offset = atof( arg2); 
			}
			else if ( !strcmp( key, "CAL_INIT")) init_cal_values();
			else if ( !strcmp( key, "CUTFILE")) read_cut_file( arg);
			else if ( !strcmp( key, "CALIBRATE")) read_cal_file( arg);
			else if ( !strcmp( key, "CAL_RUNTIME")) strcpy( runtime_calfile, arg);
			else if ( !strcmp( key, "TAGGER_BG")) { 
				tagger_background_distance = atof( arg);
				tagger_background_factor = atof( arg2);
			}
			else if (!strcmp(key,"DO_CALIBRATE")) {
				if ( atoi( arg)) control_flags |= CALIBRATE;
				else control_flags &= (~CALIBRATE);
			}
			else line[0] = 0;

			if ( line[0]) printf( "    ---> %s",line);					
		}
	}
	fclose( fp);

	// open SANE calibration initfile
	if ( ( fp = fopen( "/home/dave/asl/sane_params/ped_maximums.lookup", "r")) == NULL) {
		perror( "<E> Cannot open SANE initfile");
		fprintf( stderr,"    File = /home/dave/asl/sane_params/ped_maximums.lookup\n");
		return -1;
	}
	while ( fgets( line, sizeof( line), fp) != NULL) {
		if ( line[0] != '#') {
			sscanf( line, "%hd %hd %hd", &tadc, &lg_min, &sg_min);
			if ( ( tadc >= 1 ) && ( tadc <= 87)) {
				SANE_LG_OVERFLOW[tadc] = (float) (4093 - lg_min);
				SANE_SG_OVERFLOW[tadc] = (float) (4093 - sg_min);
			}
			else {
				fprintf( stderr, "ERROR:  problem with \"ped_maximums.lookup\".\n");
				exit( -1);
			}
		} 
	}
	fclose( fp);

	SANE_TDC_OVERFLOW = 4094;

	add_to_newfile( update_cal_values);  
	add_to_calibration( do_calibration);
  
	return 0;
}

/***************************************************************************
 * 
 * Init calibration values
 * 
 */

void init_cal_values()
{
	short bl, det;

	// Init Calibration data

	for ( bl =1; bl < BLOCKNUMBER; bl++) {         // TAPS
		for ( det = 1; det < BAFNUMBER; det++) {	
			cal.t.wide[bl][det].gain     = cal_def_wide_gain;
			cal.t.wide[bl][det].offset   = cal_def_wide_offset; 
			cal.t.narrow[bl][det].gain   = cal_def_narrow_gain;
			cal.t.narrow[bl][det].offset = cal_def_narrow_offset;
			cal.t.time[bl][det].gain     = cal_def_time_gain;
			cal.t.time[bl][det].offset   = cal_def_time_offset;
			taps_geom[bl].led_threshold[det] = 0;
		}
	}

	for( det = 1; det < MAX_FORWARD; det++) {	// Forward Detector
		cal.f.wide[det].gain      = cal_def_wide_gain;
		cal.f.wide[det].offset    = cal_def_wide_offset;
		cal.f.narrow[det].gain    = cal_def_narrow_gain;
		cal.f.narrow[det].offset  = cal_def_narrow_offset;
		cal.f.time[det].gain      = cal_def_time_gain;
		cal.f.time[det].offset    = cal_def_time_offset;
		fdet_geom.mapped_det[det] = det;
		fdet_geom.led_threshold[det] = 0;
	}	 

	for ( det = 1; det < SANE_NCELL; det++) {	// SANE
		cal.s.wide[det].gain      = cal_def_sane_wide_gain;
		cal.s.wide[det].offset    = cal_def_sane_wide_offset;
		cal.s.narrow[det].gain    = cal_def_sane_narrow_gain;
		cal.s.narrow[det].offset  = cal_def_sane_narrow_offset;
		cal.s.time[det].gain      = cal_def_sane_time_gain;
		cal.s.time[det].offset    = cal_def_sane_time_offset;
	}

	for(det=1;det<MAX_TAGGER;det++) { // Init Tagger
		tagger[det].time_win_coin_low   =  -2.00;
		tagger[det].time_win_coin_high  =   2.00;
		tagger[det].time_win_bg_low[0]  =  -8.00;
		tagger[det].time_win_bg_high[0] = -12.00;
		tagger[det].time_win_bg_low[1]  =  -8.00;
		tagger[det].time_win_bg_high[1] = -12.00;
		tagger[det].weight              =   1.00;
		cal.g.time[det].gain	       = cal_def_time_gain;
		cal.g.time[det].offset	       = cal_def_time_offset;
	}
}


/*****************************************************************************
 * 
 * Process calibration file
 * 
 * One line contains:
 * BLOCK DETECTOR MODE VAL1 VAL2
 * 
 * BLOCK:	0	TAGGER
 * 		1-6	TAPS
 * 		10	Forward Detector
 * 		20	SANE
 * MODE:		TAGGER
 * 		e	Energy			VAL1=energy
 * 						VAL2=energy width
 * 		c	Coincidence Window	VAL1=low
 * 						VAL2=high
 * 		b	Background Window	VAL1=low1
 * 						VAL2=high2
 * 		t	Time calibration	VAL1=gain
 * 						VAL2=offset
 *		(w	Scaler weight		VAL1=value)
 * 			TAPS + Forward Wall
 * 		w	Wide gate
 * 		n	Narrow gate
 *		l	LED-Threshold
 * 		t	Time			VAL1=gain
 * 						VAL2=offset
 *		m	Mapping of FW		VAL1=new number
 *
 *****************************************************************************/

void read_cal_file( char* cal_file)
{
	char line[120];
	char mode;
	float gain, offset, width, cwidth, bwidth;
	int bl, det;

	FILE *fp;

	// open initfile
	if ( ( fp = fopen( cal_file, "r")) == NULL) {
		perror( "<E> Cannot open calibration file");
		fprintf( stderr, "    File = %s\n", cal_file);
		return;
	}

	// read initfile
	while ( fgets( line, sizeof(line), fp)) {
		if ( sscanf( line, "%d %d %c %f %f", &bl, &det, &mode, &gain, &offset) > 0) {
			if ( bl == TAGG_BLOCK_NUMBER) {
				switch ( mode) {
					case 'e' :
						tagger[det].energy_center = gain;
						tagger[det].energy_width  = offset;
						break;
					case 'c' :
						tagger[det].time_win_coin_low	 = gain;
						tagger[det].time_win_coin_high = offset;
						width = (offset - gain) * tagger_background_factor;
						tagger[det].time_win_bg_low[0]  = gain - tagger_background_distance - width;
						tagger[det].time_win_bg_high[0] = gain - tagger_background_distance;
						tagger[det].time_win_bg_low[1]  =  tagger[det].time_win_bg_low[0];
						tagger[det].time_win_bg_high[1] =  tagger[det].time_win_bg_high[0];
						tagger[det].weight = tagger_background_factor;
						break;
					case 'b' :
						cwidth = tagger[det].time_win_coin_high - tagger[det].time_win_coin_low;
						tagger[det].time_win_bg_low[0]  = gain + tagger_background_factor;
						tagger[det].time_win_bg_high[0] = tagger[det].time_win_coin_low - tagger_background_distance;
						tagger[det].time_win_bg_low[1]  = tagger[det].time_win_coin_high + tagger_background_distance;
						tagger[det].time_win_bg_high[1] = offset - tagger_background_factor;
						tagger[det].weight = cwidth/(offset-gain-cwidth -2.*tagger_background_distance
								-2.*tagger_background_factor);
						break;
					case 't' :
						cal.g.time[det].gain   = gain;
						cal.g.time[det].offset = offset;
						break;
/*						case 'w' :
							tagger[det].weight = gain;
							break;
*/
				} // end switch
			} // end TAGGER
			else if ( bl == FDET_BLOCK_NUMBER) {
				switch ( mode) {
					case 'w' :
						cal.f.wide[det].gain   = gain;
						cal.f.wide[det].offset = offset;
						break;
					case 'n' :
						cal.f.narrow[det].gain   = gain;
						cal.f.narrow[det].offset = offset;
						break;
					case 't' :
						cal.f.time[det].gain   = gain;
						cal.f.time[det].offset = offset;
						break;
					case 'm' :
						fdet_geom.mapped_det[det] = gain;
						break;
					case 'l' :
						fdet_geom.led_threshold[det] = gain;
						break;
				} // end switch
			} // end FDET
			else if ( bl == SANE_BLOCK_NUMBER) {
				switch ( mode) {
					case 'w' :
						cal.s.wide[det].gain   = gain;
						cal.s.wide[det].offset = offset;
						break;
					case 'n' :
						cal.s.narrow[det].gain   = gain;
						cal.s.narrow[det].offset = offset;
						break;
					case 't' :
						cal.s.time[det].gain   = gain;
						cal.s.time[det].offset = offset;
						break;
				} // end switch
			} // end SANE
			else if ( ( bl > 0) && ( bl < BLOCKNUMBER) ) {
				switch (mode) {
					case 'w' :
						cal.t.wide[bl][det].gain   = gain;
						cal.t.wide[bl][det].offset = offset;
						break;
					case 'n' :
						cal.t.narrow[bl][det].gain   = gain;
						cal.t.narrow[bl][det].offset = offset;
						break;
					case 't' :
						cal.t.time[bl][det].gain   = gain;
						cal.t.time[bl][det].offset = offset;
						break;
					case 'l' :
						taps_geom[bl].led_threshold[det] = gain;
						break;
				} // end switch
			} // end TAPS
		} // end SCAN
	} // end while

	fclose(fp);

	return;
}

/***************************************************************************/

void read_cut_file( char* cut_file)
{
	char line[120];
	int num, dim, i;
	float x, y;

	FILE *fp;
  
	// open cutfile
	if ( ( fp = fopen( cut_file, "r")) == NULL) {
		perror( "<E> Cannot open cut definition file");
		fprintf( stderr, "    File = %s\n", cut_file);
		return;
	}

	// read cutfile
	while ( fgets( line, sizeof(line), fp)) {
		if ( line[0] != '#') {
			if ( sscanf( line, "%d %d", &num, &dim) > 0) {
				cutdef[num].dim = dim;
				for ( i = 0; i < dim; i++) {
					fgets( line, sizeof( line), fp);
					sscanf( line, "%f %f", &x, &y);
					cutdef[num].x[i] = x;
					cutdef[num].y[i] = y;
				}
			}
		}
	}

	fclose( fp);

	return;
}

/***************************************************************************
 * 
 * Update calibration values for running calibrations
 * 
 */

void update_cal_values( char* current_filename)
{
	int process;
	char line[100];
	char file[100];
	char key[40];

	FILE* fp;

	process = 0;		// process flag for calfiles

	if ( strlen( runtime_calfile)) {
    
		// open runtime file
		if ( ( fp = fopen( runtime_calfile, "r")) == NULL) {
			perror( "<E> Cannot open runtime calibration file");
			fprintf( stderr, "    File = %s\n", runtime_calfile);
			return;
		}

		// scan whole file
		while ( fgets( line, sizeof(line), fp)) {
			if ( sscanf( line, "%s %s", key, file) > 0) {
				// check filename
				if ( !strcmp( key, "FILE")) {
					if( !strncasecmp( current_filename, file, strlen( file))) {
						process = 1;
						printf( "<I> Read new calibration file(s) for file %s\n", current_filename);
					}
					else process = 0;
				}
				// calibrate in process = 1
				if ( !strcmp( key, "CALIBRATE")) {
					if ( process) {
						printf("    Use file: %s\n", file);
						read_cal_file( file);
					}
				}
			}
		} // end while

		fclose(fp);
    
	} // end if strlen()
}


/*****************************************************************************
 * 
 * Define global variables to store the histogram IDs to use
 * and fill them.
 * 
 ****************************************************************************/


int book_calibration()
{
return 0;
}


/************************************************************************
 * 
 * Calibrates the data using the calibration data read at init phase
 * 
 ************************************************************************/

void do_calibration() {

	register int tagger_channel;		// Tagger
	register int bl, det;			// BaF2
	int i;

	// TAPS BaF2
	for ( i = 1; i <= taps_hit_pointer; i++) {
		bl  = taps_hits[i].block;
		det = taps_hits[i].baf;
		taps[bl].e_long[det] = taps[bl].e_long[det]*cal.t.wide[bl][det].gain + cal.t.wide[bl][det].offset;
//		if ( taps[bl].e_long[det] < 0) taps[bl].e_long[det] = 0;
		taps[bl].e_short[det] = taps[bl].e_short[det]*cal.t.narrow[bl][det].gain + cal.t.narrow[bl][det].offset;
		if ( ( taps[bl].time[det] > 200.) && ( taps[bl].time[det] < 4000.))
			taps[bl].time[det] = taps[bl].time[det]*cal.t.time[bl][det].gain + cal.t.time[bl][det].offset; 
		else taps[bl].time[det] = -10000.;
      
		if (readout_mode == UNCOMPRESSED) {
			if (taps[bl].e_long[det] > 0) taps[bl].psa[det] = atan(taps[bl].e_short[det]/taps[bl].e_long[det])/DTORAD ;
		}				 
	}
 
	// Forward Detector
	for ( i = 1; i <= fdet_hit_pointer; i++) {
		det = fdet_hits[i].module;
		fdet.e_long[det] = fdet.e_long[det]*cal.f.wide[det].gain + cal.f.wide[det].offset;
		fdet.e_short[det] = fdet.e_short[det]*cal.f.narrow[det].gain + cal.f.narrow[det].offset;
		if ( ( fdet.time[det] > 200.) && ( fdet.time[det] < 4000.))
			fdet.time[det] = fdet.time[det]*cal.f.time[det].gain + cal.f.time[det].offset;
		else fdet.time[det] = -10000.;
	}

	// SANE
	for ( i = 1; i <= sane_cell_pointer; i++) {
		det = sane_cell_hits[i];

		sane.long_e_nocal[det] = sane.long_e[det];
		sane.short_e_nocal[det] = sane.short_e[det];
		sane.ctime_nocal[det] = sane.ctime[det];

		if ( sane.long_e[det] >= SANE_LG_OVERFLOW[det]) sane.long_e[det] = 9999;
		else if ( sane.long_e[det] > 0) sane.long_e[det] = sane.long_e[det]*cal.s.wide[det].gain + cal.s.wide[det].offset;
		else sane.long_e[det] = -9999;

		if ( sane.short_e[det] >= SANE_SG_OVERFLOW[det]) sane.short_e[det] = 9999;
		else if ( sane.short_e[det] > 0) sane.short_e[det] = sane.short_e[det]*cal.s.narrow[det].gain
			+ cal.s.narrow[det].offset;
		else sane.short_e[det] = -9999;

		if ( sane.ctime[det] >= SANE_TDC_OVERFLOW) sane.ctime[det] = 9999;
		else if ( sane.ctime[det] > 0) sane.ctime[det] = sane.ctime[det]*cal.s.time[det].gain + cal.s.time[det].offset;
		else sane.ctime[det] = -9999;
	}
	   
   	// Tagger
	// now the coincident tagger should have the same time as the photons !! First Compression only !!
	for ( i = 1; i <= tagger_pointer; i++) {
		det = tagger_hit[i];
		tagger[det].time = tagger[det].time*cal.g.time[det].gain + cal.g.time[det].offset;
	}
}
