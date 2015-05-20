/*
 * Header File including
 * 
 * Event structure of detectors added to TAPS during the MAINZ99 run
 * 
 * SANE
 *
 * 19.11.1999 SANE crap added DLH
 *
 */

// Preliminary SANE stuff -- DLH 21.11.99

#define SANE_NVETO				13					// Number of SANE vetoes + 1
#define SANE_NCOL					11					// Number of SANE columns + 1
#define SANE_CELLSEP				8.5				// Separation of SANE cells in cm
#define SANE_ACTVOL				7.6				// With of SANE-cell active volume in cm
#define MAX_SANE_SCALER			153				// Number of SANE scaler channels + 1
#define PI							3.141593			// This one's obvious
#define MPI0_MEV					134.9630			// Pi0 Mass in MeV
#define MN_MEV						939.56563		// Neutron Mass in MeV
#define MP_MEV						938.27231		// Proton Mass in MeV
#define M_DEUT_MEV				1875.587280		// Deuteron Mass in MeV
#define M_C12_MEV					11174.706055	// Carbon 12 Mass in MeV
#define EB_DEUT_MEV				2.22455001		// Deuteron Mass in MeV
#define DEGTORAD					0.0174532952   // Degrees to Radians
#define MAX_TAG_CHAN				217				// Number of Tagger Channels + 1
#define TAG_EBINS					21					// Number of Tagger Energy Bins + 1
#define PI_TH_BINS				5					// Maximum Number of Pion Theta Bins + 1
#define SMEAR						0.01				// Smearing factor

#define SANE_VALID				(1<<0)
#define SANE_VETO					(1<<1)
#define SANE_THRESH				(1<<2)
#define SANE_PION					(1<<3)
#define SANE_NEUTRON				(1<<4)

typedef struct {
  	float	short_e[SANE_NCELL];
  	float	long_e[SANE_NCELL];
  	float	ctime[SANE_NCELL];
  	float	veto_e[SANE_NVETO];
  	float	vtime[SANE_NVETO];
	float	scaler[MAX_SANE_SCALER];
	int		raw_flags[SANE_NCELL];
	int		flags[SANE_NCELL];
  	float	long_e_nocal[SANE_NCELL];
  	float	short_e_nocal[SANE_NCELL];
  	float	ctime_nocal[SANE_NCELL];
	float	psa[SANE_NCELL];
} sane_event;

typedef struct {
	float	r_cell[SANE_NCELL];			// distance to target
	float	th_cell[SANE_NCELL];		// in plane 	
	float	phi_cell[SANE_NCELL];		// out-of-plane
	float	x_cell[SANE_NCELL];			// cell placement in detector plane
  	float	y_cell[SANE_NCELL];
	float theta_res[SANE_NCELL];
	float phi_res[SANE_NCELL];
} det_sane_geom;

typedef struct {
	float m;
	float inv_m;
	float e;
	float p;
	float px;
	float py;
	float pz;
	float T;
	float theta;
	float phi;
	float time;
} sane_particle_event;

extern sane_event			sane;
extern det_sane_geom		sane_geom;
extern sane_particle_event	neutron, proton, inc_photon, pi0, deuteron;
extern sane_particle_event	neutron_m2, proton_m2;
extern sane_particle_event	kpd, kpdmq, kpdmqmn, dummy;
extern short				sane_cell_hits[SANE_NCELL];
extern short				sane_cell_pointer;
extern short				sane_veto_pointer;
extern short				sane_veto_hits[SANE_NVETO];

extern short				sane_ladc_pointer;
extern short				sane_sadc_pointer;
extern short				sane_tdc_pointer;
extern short				sane_vadc_pointer;
extern short				sane_vtdc_pointer;

int							n_scaler_events;
float							scaler_ext[353];
float							realtime_ext;
short							do_pi0_dump;
short							veto5_flag, tape_flag;
char							dfilename[100];
