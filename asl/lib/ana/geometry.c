/* Geometrie initialization 
 * 
 * 
 * Revision Date	Reason
 * ------------------------------------------------------------------------
 * 07.02.96	VH	Extracted from unpack_event.c
 * 
 * 20.02.1999	MJK	TOF Geometry added, FW geometry changed to block
 *			geometry (Mainz99)
 *
 * 19.11.1999	DLH	SANE geometry added
 *
 * $Log: geometry.c,v $
 * Revision 1.3  1998/08/04 08:07:59  hejny
 * Adding ring to fdet_geom
 *
 * Revision 1.2  1998/02/06 14:12:50  asl
 * Modules can be switched off reading compressed data
 *
 */

static char* rcsid="$Id: geometry.c,v 1.3 1998/08/04 08:07:59 hejny Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "asl.h"		/* Library header file */
#include "event.h"		/* event description and geometry */
#include "event_ext.h"		/* event description and geometry */

#if defined(OSF1) || defined(ULTRIX)	/* Platform dependencies */
# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

#define SECTION "GEOMETRY"	/* Section name */

/*****************************************************************************
 * 
 * File internal definitions
 * 
 */

void  init_taps_geom(char* line, FILE* fp);
void  init_fdet_geom(char* line, FILE* fp);
void  init_sane_geom(char* line, FILE* fp);

/*****************************************************************************/

				  	/* Initialize "geom.h" */
	
					/* Forward Detector */
forward_geom	   fdet_geom;

					/* TAPS blocks */
block_geom	   taps_geom;

det_sane_geom		sane_geom;

/*****************************************************************************
 * 
 * Structure of INITFILE:
 *
 * GEOMETRY	TAPS	6			    Number of Blocks
 *  BLOCK  1	55.0	 150.0	0.0	  0.0  0.0  0.0  Distance ,
 *  BLOCK  2	55.0	 100.0	0.0	180.0  0.0  0.0	  Theta (TAPS),
 *  BLOCK  3	55.0	  50.0	0.0	  0.0  0.0  0.0	    Phi (TAPS),
 *  BLOCK  4	55.0	 -50.0	0.0	180.0  0.0  0.0	      Rotation,
 *  BLOCK  5	55.0	-100.0	0.0	  0.0  0.0  0.0         dx,dy
 *  BLOCK -6 	55.0	-150.0	0.0	180.0  0.0  0.0       (-) = GSI size
 *
 * GEOMETRY	FDET	2 6 100.0 0.0 0.0  First, Last Ring, Distance, dx, dy
 *
 * GEOMETRY	MWPC	4      	Number of Chambers
 *  CHAMBER 1	55.0	0.0 0.0 0.0 0.0 Distance, Theta/Phi (TAPS), Rotation, dx, dy
 *
 *
 * SKIP_BAF	 5  1		Defect BaF
 * SKIP_VETO	 5  1		Defect VETO
 * SKIP_FDET	10		Defect Detector in Forward Wall
 * SKIP_PSA	 5  1		No PSA Usage (10=FW)
 *
 *
 *****************************************************************************/

int init_geometry()
{
	char    initfile[100];
	FILE    *fp;
	char    line[120];
	char    key[40], arg[80], arg2[40];
	int	bl,det,i;
                                        /* Status */
	printf("<I> Init section: %s\n",SECTION);

  
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

			if (!strcmp(key,"GEOMETRY")) {
				if (!strcmp(arg,"TAPS"))	init_taps_geom(line,fp);
				if (!strcmp(arg,"FDET"))	init_fdet_geom(line,fp);
				if (!strcmp(arg,"SANE"))	init_sane_geom(line,fp);
			}
			else if (!strcmp(key,"SKIP_BAF")) {
				taps_geom[atoi(arg)].flags[atoi(arg2)] |= NOT_USABLE;
				taps_geom[atoi(arg)].flags[atoi(arg2)] |= NOT_USABLE_EXPLICIT;
			}
			else if (!strcmp(key,"SKIP_VETO")) {
				taps_geom[atoi(arg)].flags[atoi(arg2)] |= VETO_NOT_USABLE;
				taps_geom[atoi(arg)].flags[atoi(arg2)] |= VETO_NOT_USABLE_EXPLICIT;
			}
  			else if (!strcmp(key,"SKIP_FDET")) {
				fdet_geom.flags[atoi(arg)] |= NOT_USABLE;
				fdet_geom.flags[atoi(arg)] |= NOT_USABLE_EXPLICIT;
			}
  			else if (!strcmp(key,"SKIP_FDET_VETO")) {
				fdet_geom.flags[atoi(arg)] |= VETO_NOT_USABLE;
				fdet_geom.flags[atoi(arg)] |= VETO_NOT_USABLE_EXPLICIT;
			}
			else if (!strcmp(key,"SKIP_PSA")) {
				bl  = atoi(arg);
				det = atoi(arg2);
				if ((bl>0)&&(bl<7))	taps_geom[bl].flags[det] |= NO_PSA;
				else if (bl==10)	fdet_geom.flags[det]	 |= NO_PSA;
				if ((bl>0)&&(bl<7))	taps_geom[bl].flags[det] |= NO_PSA_EXPLICIT;
				else if (bl==10)	fdet_geom.flags[det]	 |= NO_PSA_EXPLICIT;
			}
			else line[0] = 0;

			if (line[0]) printf("    ---> %s",line);					
		}
	}

	fclose(fp);

	return 0;
}

/*****************************************************************************
 * 
 * Read parameters from INIT file and calculates TAPS Geometrie
 * 
 ****************************************************************************/

void init_taps_geom(char* line, FILE* fp)
{
char	key[40],input[100];
int	blocks,block,i;
float	radius,theta,phi,omega;
int     bl,det,n;
int     ring,in_line,number,n_next;
float   x,y,z,dx,dy,x_pos,y_pos,delta_x,delta_y;
float   abs;
float	geom_factor, diameter;
  
printf("    Calculating TAPS geometry\n");

   				/* Read line GEOMETRY TAPS blocks */
sscanf(line,"%s %s %d",key,key,&blocks);

if (blocks>=BLOCKNUMBER) {
	blocks = BLOCKNUMBER - 1;
	printf("<W> To many blocks defined, set to %d\n",blocks);
	}
if (blocks<0) {
	blocks = 0;
	printf("<W> Negative block number defined, set to zero\n");
	}
if (!blocks) {
	printf("<I> Ignoring TAPS\n");
	return;
	}

   				/* Read all block parameter */
for (i=1;i<blocks+1;i++) {
	fgets( input, sizeof(input), fp);
	sscanf(input,"%s %d %f %f %f %f %f %f",key,&block,&radius
	       					,&theta,&phi,&omega
	       					,&delta_x,&delta_y);
	printf("    Block %1d: r = %6.1f"
	       " theta = %6.1f phi = %6.1f omega = %6.1f"
	       " dx = %6.1f dy=%6.1f \n",
			block,radius,theta,phi,omega,delta_x,delta_y);
	if (block<0) {
	  block = -block;
	  for(det=1;det<BAFNUMBER;det++) {	  
	    taps_geom[block].flags[det] |= GSI_SIZE;
	  }
	}
    	taps_geom[block].radius = radius;
	taps_geom[block].theta  = theta * DTORAD;
	taps_geom[block].phi    = phi   * DTORAD;
	taps_geom[block].rot    = omega * DTORAD;
        taps_geom[block].dx     = delta_x;
        taps_geom[block].dy     = delta_y;
	}

				/* Calculate block positions,
				 * rotating matrix,
				 * neighbours and detector positions */
for(bl=1;bl<BLOCKNUMBER;bl++) {
  if (taps_geom[bl].flags[1] & GSI_SIZE) {
    diameter    = 5.35;
    geom_factor = 5.35/2.*sqrt(3.);
  }
  else {
    diameter    = 6.0;
    geom_factor = 6.0/2.*sqrt(3.);
  }
  radius= taps_geom[bl].radius;
  omega = taps_geom[bl].rot;
  theta = taps_geom[bl].theta;
  phi   = taps_geom[bl].phi;
  taps_geom[bl].rot_mat_x[0]
                 = cos(omega)*cos(theta)-sin(theta)*sin(omega)*sin(phi);
  taps_geom[bl].rot_mat_x[1]
                 = sin(omega)*cos(phi);
  taps_geom[bl].rot_mat_x[2]
                 = -cos(omega)*sin(theta)-cos(theta)*sin(omega)*sin(phi);
  taps_geom[bl].rot_mat_y[0]
                 = -sin(omega)*cos(theta)-sin(theta)*cos(omega)*sin(phi);
  taps_geom[bl].rot_mat_y[1]
                 = cos(omega)*cos(phi);
  taps_geom[bl].rot_mat_y[2]
                 = sin(omega)*sin(theta)-cos(theta)*cos(omega)*sin(phi);
  taps_geom[bl].rot_mat_z[0]
                 = cos(phi)*sin(theta);
  taps_geom[bl].rot_mat_z[1]
                 = sin(phi);
  taps_geom[bl].rot_mat_z[2]
                 = cos(phi)*cos(theta);
  for(det=1;det<BAFNUMBER;det++) {

    x_pos   = 3.5*geom_factor-((det-1)%8)*geom_factor 
      		+ taps_geom[bl].dx;
    y_pos   = -3.75*diameter+((det-1)/8)*diameter+((det-1)%2)*diameter/2.
      		+ taps_geom[bl].dy;
    taps_geom[bl].x[det]   = x_pos;
    taps_geom[bl].y[det]   = y_pos;
    
				       /* calculate distance */
    x   =   x_pos*taps_geom[bl].rot_mat_x[0] +   /* by rotating block */
      y_pos*taps_geom[bl].rot_mat_y[0] +
      radius*taps_geom[bl].rot_mat_z[0];
    y   =   x_pos*taps_geom[bl].rot_mat_x[1] +
      y_pos*taps_geom[bl].rot_mat_y[1] +
      radius*taps_geom[bl].rot_mat_z[1];
    z   =   x_pos*taps_geom[bl].rot_mat_x[2] +
      y_pos*taps_geom[bl].rot_mat_y[2] +
      radius*taps_geom[bl].rot_mat_z[2];
    abs =   sqrt(x*x+y*y+z*z);
    taps_geom[bl].abs[det] = abs;                /* distance to target */

    taps_geom[bl].mtheta_taps[det] = 
                              atan2(x,z) / DTORAD;   /* -180...180 degree */
    taps_geom[bl].mphi[det]        = 
                              atan2(y,x) / DTORAD;   /* -180...180 degree */
    if (abs) {
      taps_geom[bl].mphi_taps[det]   = 
                              asin(y/abs) / DTORAD;  /*  -90...90  degree */
      taps_geom[bl].mtheta[det]      = 
                              acos(z/abs) / DTORAD;  /*    0...180 degree */
    }
    else {
      taps_geom[bl].mphi_taps[det]   = 0;
      taps_geom[bl].mtheta[det]      = 0;
    }

//  This is a check for me --- DLH 11.05.01
/*	if ( bl == 4)
	printf( "%2d  th = %4.1f  phi = %4.1f   th = %4.1f  phi = %4.1f\n", det, taps_geom[bl].mtheta[det],
			taps_geom[bl].mphi[det], taps_geom[bl].mtheta_taps[det], taps_geom[bl].mphi_taps[det]);
*/
    
                                                /* calculate neighbours */
    n=0;
    if (det<57)   taps_geom[bl].next[det][n++] = det+8;
    if (det> 8)   taps_geom[bl].next[det][n++] = det-8;
    if (((det-1)%8!=7)&&(det<64)) taps_geom[bl].next[det][n++] = det+1;
    if (((det-1)%8!=0)&&(det> 1)) taps_geom[bl].next[det][n++] = det-1;
    if ((det-1)%2) {
      if (det<58) taps_geom[bl].next[det][n++] = det+7;
      if (((det-1)%8!=7)&&(det<56)) taps_geom[bl].next[det][n++] = det+9;
    }
    else {
      if (det>7) taps_geom[bl].next[det][n++] = det-7;
      if (((det-1)%8!=0)&&(det> 9)) taps_geom[bl].next[det][n++] = det-9;
    }
    taps_geom[bl].n_next[det] = n;
  } /* end modules */
} /* end blocks */

return;
}


/*****************************************************************************
 * 
 * Calculate geometry of Forward Wall
 * 
 ****************************************************************************/

void init_fdet_geom(char* line, FILE* fp)
{
	char	key[40];
	int det, n;
	int ring, in_line, number, n_next;
	int real_number;		// after mapping
	int first, last;
	float radius, x, y, z, dx, dy, x_pos, y_pos;
	float delta_x, delta_y, rotate;
	float abs, r_abs, x0, y0, diameter, geom_factor;
//	float omega, theta, phi;

	printf("    Calculating FDET geometry\n");

	// Read line GEOMETRY FDET first last dist
	sscanf( line, "%s %s %d %d %f %f %f %f", key, key, &first, &last, &radius, &delta_x, &delta_y, &rotate);

	printf("    Radius = %5.1f, rings from %d to %d,\n    dx = %6.1f, dy = %6.1f, rotate = %5.2f\n", radius, first, last,
			delta_x, delta_y, rotate);
	fflush(stdout);
    
	fdet_geom.radius = radius;
	fdet_geom.first_ring = first;
	fdet_geom.last_ring = last;
	fdet_geom.dx = delta_x;
	fdet_geom.dy = delta_y;
	fdet_geom.rotate = rotate;
	rotate = rotate*DTORAD;
  
	// Geometry of forward detector
	radius = fdet_geom.radius;
	number = 1;
	diameter = 6.0;
	geom_factor = 6.0/2.*sqrt(3.);

/*	omega = fdet_geom.rot;
	theta = fdet_geom.theta;
	phi   = fdet_geom.phi;*/
/*	omega = 0;
	theta = 0;
	phi = 0;*/
  
/*	fdet_geom.rot_mat_x[0] = cos(omega)*cos(theta)-sin(theta)*sin(omega)*sin(phi);
	fdet_geom.rot_mat_x[1] = sin(omega)*cos(phi);
	fdet_geom.rot_mat_x[2] = -cos(omega)*sin(theta)-cos(theta)*sin(omega)*sin(phi);
	fdet_geom.rot_mat_y[0] = -sin(omega)*cos(theta)-sin(theta)*cos(omega)*sin(phi);
	fdet_geom.rot_mat_y[1] = cos(omega)*cos(phi);
	fdet_geom.rot_mat_y[2] = sin(omega)*sin(theta)-cos(theta)*cos(omega)*sin(phi);
	fdet_geom.rot_mat_z[0] = cos(phi)*sin(theta);
	fdet_geom.rot_mat_z[1] = sin(phi);
	fdet_geom.rot_mat_z[2] = cos(phi)*cos(theta);*/

	number=0;
	for ( det = 1; det < MAX_FORWARD + 7; det++) {
		if ( ( det != 61) && ( ( det < 71) || ( det > 73)) && (( det < 82) || ( det > 84)) ) {      
			number++;
			x_pos = 5*geom_factor-((det-1)%11)*geom_factor + fdet_geom.dx;
			y_pos = -6.5*diameter+((det-1)/11)*diameter+(((det-1)%11)%2)*diameter/2. + fdet_geom.dy;
			if ( number > 137) {
				x_pos -= geom_factor*(number - 137);
				y_pos -= diameter/2.*((number - 137)%2);
			}
			real_number = fdet_geom.mapped_det[number];
//			printf("det %d num %d %d x %f y %f \n", det, number, real_number, x_pos, y_pos);
			fdet_geom.x[real_number]   = x_pos;
			fdet_geom.y[real_number]   = y_pos;    

			// calculate distance by rotating block
/*			x   =   x_pos*fdet_geom.rot_mat_x[0] +   
			y_pos*fdet_geom.rot_mat_y[0] +
			radius*fdet_geom.rot_mat_z[0];
			y   =   x_pos*fdet_geom.rot_mat_x[1] +
			y_pos*fdet_geom.rot_mat_y[1] +
			radius*fdet_geom.rot_mat_z[1];
			z   =   x_pos*fdet_geom.rot_mat_x[2] +
			y_pos*fdet_geom.rot_mat_y[2] +
			radius*fdet_geom.rot_mat_z[2];
*/
			x = x_pos;
			y = y_pos;
			z = radius;
			abs = sqrt(x*x+y*y+z*z);
			fdet_geom.abs[real_number] = abs;							// distance to target
			fdet_geom.mtheta_taps[real_number] = atan2(x,z) / DTORAD;		// -180...180 degree
			fdet_geom.mphi[real_number] = atan2(y,x) / DTORAD;			// -180...180 degree
			if (abs) {
				fdet_geom.mphi_taps[real_number] = asin(y/abs) / DTORAD;	//  -90...90  degree
				fdet_geom.mtheta[real_number] = acos(z/abs) / DTORAD;		//    0...180 degree
			}
			else {
				fdet_geom.mphi_taps[real_number]   = 0;
				fdet_geom.mtheta[real_number]      = 0;
			}
		}
	}  
  
	fdet_geom.maxdet = number + 1;     // store number of modules used

	printf(" = %d modules\n",number);

	for ( number = 1; number < fdet_geom.maxdet; number++) {
		n_next = 0;
		for ( n = 1; n < fdet_geom.maxdet; n++) {
			if ( n != number) {
				dx = fdet_geom.x[number] - fdet_geom.x[n];
				dy = fdet_geom.y[number] - fdet_geom.y[n];
				if ( ( dx*dx + dy*dy) < 50.0 ) {	   
					if ( n_next > 5) printf( "<W> Got more than 6 FW neighbours ! %d\n", n_next);
					fdet_geom.next[number][n_next++] = n;
				}	   
			}
		}
		fdet_geom.n_next[number] = n_next;
	}

	return;
}

/***************************************************************************
 * 	calculate SANE geometry 					   *
 * *************************************************************************/

struct SCOLUMN {
	short num[11];
	short adc[11];
	short veto[11];
} scol[11];


void init_sane_geom( char* line, FILE* fp)
{
	short i, j;
	short tadc, nadc;
	short det;
	float r0, theta0;
	float x[11];
	char key[32];

	FILE *in;

	printf( "    Calculating SANE geometry\n");

   	// Read line GEOMETRY SANE cells
	sscanf( line,"%s %s %f %f", key, key, &r0, &theta0);
	printf( "       r0 = %5.1f  theta0 = %4.1f\n", r0, theta0);

// read-in for lookups
	i = 0;
	j = 1;
	if ( ( in = fopen( "/home/dave/asl/sane_params/all_num.lookup", "r")) == NULL) {
		fprintf( stderr, "ERROR:  problem opening \"all_num.lookup\".\n");
		exit( -1);
	}
	while ( fgets( key, sizeof( key), in) != NULL) {
		sscanf( key, "%hd %hd", &tadc, &nadc);
		if ( ( tadc >= 0 ) && ( tadc <= 87)) {
			scol[++i].adc[j] = tadc;
			scol[i].num[j] = nadc;
			if ( i == 10) {
				i = 0;
				j++;
			}
		}
		else {
			fprintf( stderr, "ERROR:  problem with \"all_num.lookup\".\n");
			exit( -1);
		}
	}
	fclose( in);

	for ( i = 1; i <= 10; i++) {
		if ( i <= 5) {
			x[i] = -SANE_CELLSEP*(0.5 + (5 - i));
		}
		else {
			x[i] = SANE_CELLSEP*(0.5 + (i - 6));
		}
//		printf( "x[%2d] = %6.2f\n", i, x[i]);
	}

	for ( i = 1; i <= 10; i++) {
		for ( j = 1; j <= 10; j++) {
			det = scol[j].adc[i];
			sane_geom.x_cell[det] = x[j];
			sane_geom.y_cell[det] = x[11-i];

			sane_geom.r_cell[det] = sqrt( r0*r0 + x[j]*x[j] + x[11-i]*x[11-i]);
			sane_geom.th_cell[det] = theta0 + atan( x[j]/r0)/DEGTORAD;

			/*
			if ( i < 6) sane_geom.phi_cell[det] = 180 - atan( x[11-i]/r0)/DEGTORAD;
			else sane_geom.phi_cell[det] = -180 - atan( x[11-i]/r0)/DEGTORAD;
			*/

			if ( i < 6) 
				sane_geom.phi_cell[det] = 180 - sane_geom.y_cell[det]/sane_geom.r_cell[det]
					/sin( sane_geom.th_cell[det]*DEGTORAD)/DEGTORAD;
			else 
				sane_geom.phi_cell[det] = -180 - sane_geom.y_cell[det]/sane_geom.r_cell[det]
					/sin( sane_geom.th_cell[det]*DEGTORAD)/DEGTORAD;

			sane_geom.theta_res[det] = 2*atan2( SANE_ACTVOL, 2*sane_geom.r_cell[det])/DEGTORAD;
			// This I don't use.  See "sane.c."
			sane_geom.phi_res[det] = asin( SANE_ACTVOL/sane_geom.r_cell[det]/sin( sane_geom.th_cell[det]*DEGTORAD))/DEGTORAD;
		}
	}

	printf( "SANE GEOMETRY\n");
	printf( "-----------------------------------------\n");
	printf( "cell x-position in frame\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %6.2f", sane_geom.x_cell[scol[j].adc[i]]);
			else printf( "       ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell y-position in frame\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %6.2f", sane_geom.y_cell[scol[j].adc[i]]);
			else printf( "       ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell distance from target\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %5.1f", sane_geom.r_cell[scol[j].adc[i]]);
			else printf( "      ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell theta w.r.t. the target\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %5.2f", sane_geom.th_cell[scol[j].adc[i]]);
			else printf( "      ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell theta resolution\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %5.2f", sane_geom.theta_res[scol[j].adc[i]]);
			else printf( "      ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell phi w.r.t. the target\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %7.2f", sane_geom.phi_cell[scol[j].adc[i]]);
			else printf( "        ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "cell phi resolution\n");
	for ( i = 1; i <= 10; i++) {
		printf( "  ");
		for ( j = 1; j <= 10; j++) {
			if ( scol[j].adc[i] != 0) printf (" %7.2f", sane_geom.phi_res[scol[j].adc[i]]);
			else printf( "        ");
		}
		printf( "\n");
	}
	printf( "-----------------------------------------\n");
	printf( "-----------------------------------------\n");
	{
		float rr, r_sq_inv, tot;
		tot = 0;
		r_sq_inv = 0;
		printf( "Neutron Solid Angle Calculation\n");
		for ( i = 1; i <= 10; i++) {
			for ( j = 1; j <= 10; j++) {
				if ( scol[j].adc[i] != 0) {
					rr = sane_geom.r_cell[scol[j].adc[i]];
					tot += 1/SQR(rr);
//					if (( j >= 3) && ( j <= 7)) {
					if (( j >= 8) && ( j <= 10)) {
						if (( i >= 3) && ( i <= 8)) {
//							printf ( " col = %2d  row = %2d  r = %6.2f\n", j, i, rr);
							r_sq_inv += 1/SQR(rr);
						}
					}
				}
			}
		}
		printf( "r_sq_inv =  %8.6f\n", r_sq_inv);
		printf( "Solid Angle =  %7.5f\n", r_sq_inv*SQR( 8.5));
		printf( "Total Solid Angle =  %7.5f\n", tot*SQR( 8.5));
	}
	printf( "-----------------------------------------\n");
}
