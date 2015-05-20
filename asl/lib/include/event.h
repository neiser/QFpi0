
/*
 * Header File including
 * 
 * Event structure
 * Geometry
 * Tagger
 * Calibration
 * 
 * $Id: event.h,v 1.3 1998/08/04 08:08:12 hejny Exp $
 * 
 * $Log: event.h,v $
 * Revision 1.3  1998/08/04 08:08:12  hejny
 * Adding ring to fdet_geom
 *
 * Revision 1.2  1998/02/06 14:12:52  asl
 * Modules can be switched off reading compressed data
 *
 * 06.04.1999 Adjustment for MAINZ99 Setup   MJK
 *
 * 09.08.1999 moved NAI, MWPC, NTOF to event_ext.h   MJK
 */

#ifndef _EVENT_HEADER
#define _EVENT_HEADER

#define MAX_FORWARD			143		// Maximum number of forward modules
#define SANE_NCELL			88		// Maximum number of SANE modules + 1
#define BLOCKNUMBER			7		// Number of BaF-Blocks
#define BAFNUMBER			65		// Number of BaFs per Block
#define MAX_LATCH			25		// Number of INT for latch
#define MAX_TAGGER			353		// Number of tagger channels
#define MAX_MICROSCOPE		97		// Number of microscope channels
#define MAX_MICRO_LOG		193 		// Number of logical micro channels
#define MAX_CAMAC_SCALER		25		// Number of tagger camac scaler
#define MAX_PART_NUMBER		100		// Maximum of invariant masses
#define MAX_CLUS_NUMBER		50		// Maximum of clusters
#define SUBEVENT_NUMBERS		256
#define FDET_BLOCK_NUMBER	10		// Block number used for FDET
#define TAGG_BLOCK_NUMBER	0		// Block number used for Tagger
#define SANE_BLOCK_NUMBER	20		// Block number used for SANE
#define GEANT_TAGGER		21		// Tagger channel for GEANT
#define MAX_TRIGGER_SCALER	97		// Number of Trigger Scaler
#define MAX_GEANT_PARTICLES	100		// Maximum Particles in GEANT
#define MAX_LASER_WIDE		8
#define MAX_LASER_NARROW		8
#define MAX_DIODE			3
#define MAX_TOT_SCALER		1000		// maximum possible number of scalers

#define HIT					(1<<0)		// Module hit
#define IN_CLUSTER				(1<<1)		// already member of cluster
#define VETO					(1<<2)		// Veto has fired
#define PHOTON					(1<<3)		// May be a photon
#define NEUTRAL				(1<<4)		//        a neutral
#define CHARGED				(1<<5)		//        a charged
#define ANYTHING				(1<<6)		//        anything
#define VALID_MEMBER			(1<<7)		// already checked cluster member
#define NOT_USABLE				(1<<8)		// baf off
#define VETO_CHECKED			(1<<9)		// veto examined
#define VETO_NOT_USABLE			(1<<10)		// veto off
#define BACKGROUND				(1<<11)
#define COINCIDENT				(1<<12)		// photon coincident with others
#define PROTON					(1<<13)		// May be a proton
#define NEUTRON				(1<<14)		//        a neutron
#define DEUTERON				(1<<15)		//        a deuteron
#define PION					(1<<16)		// That's a pion
#define ETA					(1<<17)		// That's a eta
#define LED					(1<<18)		// LED has fired
#define VETO_READOUT			(1<<19)		// Veto energy has been readout
#define GSI_SIZE				(1<<20)		// Module has diameter 5.5 mm
#define NO_PSA					(1<<21)		// Don't use Pulseshape
#define TIMEWARP				(1<<22)		// Indicates a TimeWarp Detector
#define TW_CHECKED				(1<<23)		// Indicates if module is checked
#define NOT_USABLE_EXPLICIT		(1<<24)		// baf off
#define VETO_NOT_USABLE_EXPLICIT	(1<<25) 		// veto off
#define NO_PSA_EXPLICIT			(1<<26) 		// Don't use Pulseshape

// TAGGER status
#define TAGGER_VALID		(1<<0)
#define TAGGER_TDC			(1<<1)
#define TAGGER_LATCH		(1<<2)
#define TAGGER_CLUSTER		(1<<3)
#define TAGGER_COINCIDENT	(1<<4)
#define TAGGER_BACKGROUND	(1<<5)

// Geometry of forward detector
typedef struct {
	short	maxdet;			// number of detectors
	short	first_ring;
	short	last_ring;		// rings to construct
	float	radius;			// distance to target
//	float	theta;			// in-plane
//	float	phi;				// out-of-plane
//	float	rot;				// rotation of block
	float	dx;				// FW-Shift in x
	float	dy;				// FW-Shift in y
	float	rotate;
	float	x[MAX_FORWARD];	// module coordinates
	float	y[MAX_FORWARD];
	float	z[MAX_FORWARD];
/*	float	rot_mat_x[3];		// rotation matrix 
	float	rot_mat_y[3];	
	float	rot_mat_z[3];
*/
	float	mtheta[MAX_FORWARD];
	float	mphi[MAX_FORWARD];
	float	mtheta_taps[MAX_FORWARD];	// theta, phi in taps geometry
	float	mphi_taps[MAX_FORWARD];
     float	abs[MAX_FORWARD];			// distance to target
	short	next[MAX_FORWARD][6];		// modules in first ring
	short	n_next[MAX_FORWARD];		// number of modules
	int		flags[MAX_FORWARD];			// Flag variable, meaning above
	short	mapped_det[MAX_FORWARD];		// mapped position
	float	led_threshold[MAX_FORWARD];
	short	ring[MAX_FORWARD];			// detector in which ring?
} forward_geom;

// TAPS Forward Events
typedef struct {
	short	hits;					// Number of hits
	float	e_short[MAX_FORWARD];		// Narrow Gate
	float	e_long[MAX_FORWARD];		// Wide Gate
	float	time[MAX_FORWARD];			// Time
	float	psa[MAX_FORWARD];			// Pulse Shape (not used)
	int		flags[MAX_FORWARD];			// Flag variable, meaning above
	int		led_pattern[6];			// LED Pattern
	int		veto_pattern[6];			// Veto Pattern
} forward_event;

// hit buffer
typedef struct  {
        short   module;                 // Detector hit
} forward_hit_buffer[MAX_FORWARD];

// Geometry of each defined block
typedef struct {
	float	radius;			// distance to target
	float	theta;			// in-plane
	float	phi;				// out-of-plane
	float	rot;				// rotation of block
							// GSI  :  90 degree
							// Mainz:   0 degree
							//        180 degree
	float	dx;				// Block-Shift in x
	float	dy;				// Block-Shift in y
	float	x[BAFNUMBER];		// plain blocks
	float	y[BAFNUMBER];
	float	mtheta[BAFNUMBER];
	float	mphi[BAFNUMBER];
	float	mtheta_taps[BAFNUMBER];	// theta, phi in taps geometry
	float	mphi_taps[BAFNUMBER];
	float	rot_mat_x[3];			// rotation matrix 
	float	rot_mat_y[3];	
	float	rot_mat_z[3];	
	float	abs[BAFNUMBER];		// distance to target
	short	next[BAFNUMBER][6];		// modules in first ring
	short	n_next[BAFNUMBER];		// number of neighbours
	int		flags[BAFNUMBER];			// Flag variable, meaning above
	float   led_threshold[BAFNUMBER];
} block_geom[BLOCKNUMBER];

// TAPS BaF2 Events
typedef struct {
	short	hits;                   // Number of hits in this block
	float	e_short[BAFNUMBER];     // Narrow Gate
	float	e_long[BAFNUMBER];      // Wide Gate
	float	time[BAFNUMBER];        // Time
	float	psa[BAFNUMBER];         // PSA
	float	e_veto[BAFNUMBER];	// Veto Energies
	int		flags[BAFNUMBER];	// Flag variable, meaning above
	int		led_pattern[2];		// LED Pattern
	int		veto_pattern[2];	// Veto Pattern
} block_event[BLOCKNUMBER];

// hit buffer
typedef struct  {
	short    block;                  // Block hit
	short    baf;                    // Detector hit
} block_hit_buffer[BAFNUMBER*BLOCKNUMBER];

// laser event
typedef struct  {
	float	wide[MAX_LASER_WIDE];
	float	narrow[MAX_LASER_NARROW];
	float	diode[MAX_DIODE];
} laser_event;

// tagger channels
typedef struct {
	float	time;			// time
	float	time_corrected;	// time
	float	energy;			// energy
	float	energy_width;		// channel width
	float	energy_center;		// channel center energy
	float	time_win_coin_low;	// baf_tagger_coincidence
	float	time_win_coin_high;
	float	time_win_bg_low[2];	// baf_tagger background
	float	time_win_bg_high[2];
	float	weight;			// scaled weight
	float	scaler;			// recent scaler value
	float	last_scaler;		// last scaler value
	int		type;			// type (see above)
} tagger_event[MAX_TAGGER];

typedef struct {
  	float	time;
  	float	energy;
	float	time_win_coin_low;	// baf_tagger_coincidence
	float	time_win_coin_high;
	float	time_win_bg_low[2];	// baf_tagger background
	float	time_win_bg_high[2];
	float	weight;			// scaled weight
  	int		flags;
	float	scaler;
	float	last_scaler;
} microscope_event[MAX_MICROSCOPE];

typedef struct {
        float   time;
        float   energy;
        float   time_win_coin_low;      // baf_tagger_coincidence
        float   time_win_coin_high;
        float   time_win_bg_low[2];     // baf_tagger background
        float   time_win_bg_high[2];
        float   weight;                 // scaled weight
        int     flags;
        float   scaler;
        float   last_scaler;
} micro_log_event[MAX_MICRO_LOG];      

typedef struct {
  	int		event_number;
  	int		particle_number;
  	int		id[MAX_GEANT_PARTICLES];
  	float	x[MAX_GEANT_PARTICLES];
  	float	y[MAX_GEANT_PARTICLES];
  	float	z[MAX_GEANT_PARTICLES];
  	float	px[MAX_GEANT_PARTICLES];
  	float	py[MAX_GEANT_PARTICLES];
  	float	pz[MAX_GEANT_PARTICLES];
	float	theta[MAX_GEANT_PARTICLES];
  	float	phi[MAX_GEANT_PARTICLES];
} geant_event; 

typedef struct {
	float	x;				// Position center of cluster
	float	y;
	float	z;
  	float	radius;
	float	theta;
	float	phi;
	float	theta_taps;		// theta, phi in taps geometry
	float	phi_taps;
	float	abs;				// distance to target
	short	number;    		// number of detectors in cluster
	short	block;			// block number
	short	baf[MAX_FORWARD];	// cluster members
	int		type;			// cluster type (see above)
	float	energy;			// energy sum
	float	time;			// time of used central detector
	float	time_mean;		// mean time of cluster
	float	psa;				// Pulseshape of central
	int   	lm_baf[2];		// Two highest local maxima
	int		lm_type[2];		// module and type
	float	lm_energy[2];
	float	lm_time[2];
} single_cluster_buffer;

// definition of maximum number
// of clusters
typedef single_cluster_buffer cluster_buffer[MAX_CLUS_NUMBER];

// primary particles
typedef struct {
	float	x;			// Position center of cluster
	float	y;			// Vector normalized
	float	z;
  	float	radius;		// dist to block/fw center
	float	theta;
	float	phi;
	float	theta_taps;	// theta, phi in taps geometry
	float	phi_taps;
	float	abs;			// distance to target
	float	p;			// momentum
	float	energy;		// energy sum
	float	tof_energy;	// energy calculated from TOF
	float	time;		// time of central detector
	float	time_mean;	// mean time of cluster
	float	c_time;		// photon time for abs
	float	psa;			// Pulseshape of central
	short	block;		// Block
	short	id;			// Block*64+lm_baf[0]
	short	number;		// Number of modules
	int		type;		// what can it be else ?
} primary_particle;

// reconstructed particle
typedef struct {
	int	 	type;		// type flags (see above)
	float	x;			// Direction of flight
	float	y;
	float	z;
	float	theta;
	float	phi;
	float	theta_taps;	// theta, phi in taps geometry
	float	phi_taps;
	float	p;			// momentum
	float	px;
	float	py;
	float	pz;
	float	e;			// energy
	float	m;			// mass
	float	time;
						// only for two-gamma
	float	psi[3];		// opening angle
	short	primary[3];	// number of used particles
} particle;

// pair of calibration data
typedef struct {
	float	gain;			// result =		   
	float	offset;			//   offset + gain * value 
} calibration_data;

// all calibration info
typedef struct {
	  struct {			// TAPS
		calibration_data time[BLOCKNUMBER][BAFNUMBER];
		calibration_data wide[BLOCKNUMBER][BAFNUMBER];
		calibration_data narrow[BLOCKNUMBER][BAFNUMBER]; 
		} t;
	  struct {			// Forward Detector
		calibration_data time[MAX_FORWARD];
		calibration_data wide[MAX_FORWARD];
		calibration_data narrow[MAX_FORWARD];
		} f;
	  struct {			// Tagger
	  	calibration_data time[MAX_TAGGER];
		} g;
  	  struct {
	    	calibration_data time[MAX_MICROSCOPE];
	        } m; 
	  struct {			// SANE
		calibration_data time[SANE_NCELL];
		calibration_data wide[SANE_NCELL];
		calibration_data narrow[SANE_NCELL];
		} s;
	} calibration_parameter;

extern tagger_event		tagger;		         // Tagger
extern short			tagger_hit[MAX_TAGGER];
extern short			tagger_pointer;
extern short			tagger_wr_hit[MAX_TAGGER];
extern short			tagger_wr_pointer;
extern float			tagger_background_distance;
extern float			tagger_background_factor;
extern int			tagger_latch[MAX_LATCH];
extern unsigned int		last_scaler_event;
extern unsigned int		current_scaler_event;
extern int			camac_scaler[MAX_CAMAC_SCALER];
extern unsigned int		global_scaler[MAX_TOT_SCALER];
extern float			lead_glas;
extern float			lead_glas2;
extern float			lead_glas3;

extern microscope_event	micro;
extern short			micro_hit[MAX_MICROSCOPE];
extern short			micro_pointer;
extern micro_log_event	micro_log;
extern short			micro_hit_log[MAX_MICRO_LOG];
extern short			micro_pointer_log;

extern int			L_hit_pattern;
extern float			pileup_tdc[9];

extern forward_event		fdet;		         // Forward Detektor
extern forward_geom			fdet_geom;	
extern forward_hit_buffer	fdet_hits;
extern short				fdet_hit_pointer;

extern block_event		taps;                  // TAPS Blocks
extern block_geom		taps_geom;
extern block_hit_buffer	taps_hits;
extern short			taps_hit_pointer;      // belongs to hits

extern laser_event		laser;		// laser event

extern geant_event		geant;		// geant event

extern cluster_buffer		cluster;
extern short				cluster_pointer;  // belongs to cluster

extern primary_particle		photon[MAX_CLUS_NUMBER];
extern int				photon_pointer;   // belongs to photon[]

extern primary_particle		neutral[MAX_CLUS_NUMBER];
extern int				neutral_pointer;  // belongs to neutral[]

extern primary_particle		charged[MAX_CLUS_NUMBER];
extern int				charged_pointer;  // belongs to charged[]

extern primary_particle		unknown[MAX_CLUS_NUMBER];
extern int				unknown_pointer;  // belongs to unknown[]

extern particle		invmass[MAX_PART_NUMBER];
extern particle		dilepton[MAX_PART_NUMBER];
extern particle		dalitz[MAX_PART_NUMBER];
extern short			inv_mult;
extern short			dilep_mult;
extern short			dalitz_mult;
extern short			pi_mult;	    // identified
extern short			eta_mult;

extern calibration_parameter cal;	    // Calibration Structure

extern int		  event_number;	// Event number
extern int		  event_valid;		// Event number
extern int		  event_unsaved;	// Events since last save

extern int		  run_event_number;
extern int		  run_event_valid;

extern unsigned int	  current_buffer_number;
extern unsigned int	  start_buffer_number;
extern unsigned int	  start_event_number;
extern unsigned int	  events_per_buffer;

extern unsigned int	  trigger_pattern;  // Trigger Pattern
extern unsigned int	  trigger_pattern2;
extern unsigned int	  trigger_mask_0;
extern unsigned int	  trigger_mask_1;
extern unsigned int	  trigger_scaler[MAX_TRIGGER_SCALER];
extern unsigned int	  num_trigger_scaler;

					    // Subevents to be skipped ...
extern short              skip_subevent[SUBEVENT_NUMBERS]; 
extern int                moni_io;	    // Monitor each ... event
extern float              e_gamma;	    // for GEANT simulation

extern unsigned int	  event_status;	    // Event status register

					// Event Status Register
#define ESR_COUNT_FRAGMENT_BIT			30
#define ESR_COUNT_EVENT_BIT				31

#define ESR_SCALER_EVENT					(1<<0) // Event contents tagger scaler
#define ESR_NEW_FILE					(1<<1) // First valid (?) event of new file
#define ESR_VALID_EVENT					(1<<2) // Indicates a valid event
#define ESR_FRAGMENTED					(1<<3) // Event is fragmented
#define ESR_TRIGGER_SCALER_EVENT			(1<<4) // Event contains ...
#define ESR_NO_PHOTON_COINCIDENCE			(1<<5) // No photon coincidence
#define ESR_TOO_MANY_PHOTON_COINCIDENCE		(1<<6) 
#define ESR_NO_TAGGER_COINCIDENCE			(1<<7) // No tagger coincidence
#define ESR_TOO_MANY_TAGGER_COINCIDENCE		(1<<8) 
#define ESR_IN_COIN_WINDOW				(1<<9) 
#define ESR_SIMPLE_COINCIDENCE			(1<<10) 
#define ESR_VALID_SCALER					(1<<11) // Compression ...
#define ESR_PACK_EVENT					(1<<12) 
#define ESR_MICROSCOPE_EVENT				(1<<13) // Microscope Event
#define ESR_PION_DET_EVENT				(1<<16) // Pion Detector Event
#define ESR_TOF_DET_EVENT				(1<<17) // TOF Detector Event
#define ESR_PLASTIC_DET_EVENT				(1<<18) // Plastic det Event
#define ESR_SANE_DET_EVENT				(1<<19) // SANE Detector Event
#define ESR_NO_POLARISATION				(1<<20)
#define ESR_POLARISATION_1				(1<<21)
#define ESR_POLARISATION_2				(1<<22)
#define ESR_NEW_RUN						(1<<23)
#define ESR_IGNORE_SCALER				(1<<29) // Ignore Scaler Event
				       // Entry bits in event.status
				       // for fragment and event number
#define ESR_COUNT_FRAGMENT	 (1<<ESR_COUNT_FRAGMENT_BIT) 
#define ESR_COUNT_EVENT		 (1<<ESR_COUNT_EVENT_BIT) 

#define ECS_IGNORE_SCALER	1
#define ECS_POLARISATION_1	2
#define ECS_POLARISATION_2	4


				       // Trigger Bits

#define TB_CFD_SA		(1<<0)	
#define TB_LED_LOW_SA	(1<<1)
#define TB_LED_HIGH_SA	(1<<2)
#define TB_CFD			(1<<3)
#define TB_LED_LOW		(1<<4)
#define TB_LED_HIGH		(1<<5)
#define TB_DIODE_SA		(1<<6)
#define TB_LASER_SA		(1<<7)
#define TB_POL_1		(1<<8)	// Polarisation 1
#define TB_POL_2		(1<<9)	// Polarisation 2
#define TB_2_GAMMA		(1<<10)	// photon_pointer >= 2
#define TB_3_GAMMA		(1<<11)	// photon_pointer >= 3
#define TB_4_GAMMA		(1<<12)	// photon_pointer >= 4
#define TB_CHARGED		(1<<13)	// charged_pointer > 0
#define TB_NEUTRAL		(1<<14)	// neutral_pointer > 0
#define TB_PION		(1<<15)	// pi_mult > 0
#define TB_ETA			(1<<16)	// eta_mult > 0
#define TB_LASER		(1<<21)
#define TB_TAGGER		(1<<23)
#define TB_SOFTTRIGGER	(1<<30)
#define TB_SCALER	    	(1<<31)

//  Compression Trigger Bits
#define CP_VETO		(1<<0)		// Veto has fired
#define CP_LED			(1<<1)		// Energy > LED (software)
#define CP_LED_H		(1<<2)		// Energy > LED (hardware)
#define CP_OFF			(1<<3)		// Module not usable at all
#define CP_VETO_OFF		(1<<4)		// Veto not usable
#define CP_PSA_OFF		(1<<5)		// Pulseshape not usable
#define CP_TIMEWARP		(1<<6)		// Corrected for timewarp

#endif
