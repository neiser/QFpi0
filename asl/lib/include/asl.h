/*
 * $Id: asl.h,v 1.10 1998/08/04 08:46:18 hejny Exp $
 * 
 * $Log: asl.h,v $
 * Revision 1.10  1998/08/04 08:46:18  hejny
 * define LIBC6 added for such systems
 *
 * Revision 1.9  1998/08/04 08:08:55  hejny
 * bug fixes and changes for Debian 2.0
 *
 * Revision 1.8  1998/05/12 15:23:13  hejny
 * cut parameters implemented
 *
 * Revision 1.7  1998/04/28 08:46:25  hejny
 * *** empty log message ***
 *
 * Revision 1.6  1998/04/02 10:02:30  hejny
 * Added Veto-only Modules
 *
 * Revision 1.5  1998/03/25 10:45:30  hejny
 * Added error handling for 2D histograms
 *
 * Revision 1.4  1998/02/16 16:58:57  hejny
 * Show can work without initfile
 *
 * Revision 1.3  1998/02/16 16:33:28  hejny
 * Bigfix: V2.4B and earlier are readable again
 *
 * Revision 1.2  1998/01/28 17:26:19  hejny
 * Adding profile histograms to the analysis.
 *
 */


#ifndef __ASLHEADER
#define __ASLHEADER

#define ASL_VERSION	"ASL Version 2.7b, 1994-98 by VH,..."

#if defined(OSF1) || defined(ULTRIX)
# include <sys/types.h>          /* network access ... */
# include <netinet/in.h>
# include <netdb.h>
# include <sys/socket.h>         /* sockets ... */
# include <fcntl.h>
# include <errno.h>              /* Error handling */
# include <sys/ioctl.h>		 /* for taping */
# include "tapedefs.h"
# if defined(LINUX)
   int xargc;			 /* for g77 */
   char **xargv;
   int __argc_save;		 /* for pgf77 */
   char** __argv_save;
#  if !defined(LIBC6) 
#   include <linux/time.h> 	 /* You will need this for libc5-Systems */
#  endif
#include <string.h>
# endif
# if defined(AIX)
#  include <sys/tape.h>
#  include <sys/select.h>
# else
#  include <sys/mtio.h>
# endif
#elif defined(VMS)
# include <ctype.h>
# include <socket.h>
# include <types.h>
# include <in.h>
# include <netdb.h>
# include <file.h>
#endif


#if !defined(NO_CFORTRAN)
# include "init_cfortran.h"
#endif

#if defined(VMS)
/************************************************************
 *                             MACROS_BITOP.H
 *
 * description:    defines a few macros for bit-operations 
 * 		   necessary for select() in tapas_testana.c
 *
 *
 * author:         Martin Appenheimer (PIG)
 * date:           12.11.93
 *************************************************************/


#define MAX_NOFILE      4096    		/* This should not exist ! */
#define FD_SETSIZE      MAX_NOFILE

typedef long    fd_mask;

#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif /* howmany */

typedef struct fd_set {
        fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define NBBY 8
#define NFDBITS (sizeof(fd_mask) *NBBY)         /* bits per mask */
#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      memset((char *)(p) ,0 , sizeof(*(p)))
#endif

#if defined(OSF1) || defined(ULTRIX)
# define LOGFILE	"./logfile.adm"
# define STATUSFILE	"./statusfile.adm"
#elif defined(VMS)
# define LOGFILE	"logfile.adm"
# define STATUSFILE	"statusfile.adm"
#endif

#define SERVER_PORT	9011

#define	MAX_SECTION	100
#define MAX_ANA		100
#define MAX_HISTO       30000
#define MAX_PICTURE     100
//#define MAX_ARRAY	100
//#define MAX_ARRAY	200
#define MAX_ARRAY	30000

                        /* analyses status */
#define A_RUNNING   (1<<0)
#define A_SLEEPING	(1<<1)
#define A_ABORTED	(1<<2)
#define A_SECURE	(1<<3)
#define A_AUTOABORT	(1<<4)
#define A_STABLE	(1<<5)
#define A_SEEN		(1<<6)
#define A_SPEC_CLEARED  (1<<7)
#define A_SPEC_SAVED    (1<<8)

#define	HEADER_SIZE	10
#define CBUFFER_SIZE	8192
#define SBUFFER_SIZE	4096
#define IBUFFER_SIZE	2048

				/* header entries */
#define COMMAND		0
#define BUFFER_MODE	7
#define BUFFER_LENGTH	8
#define BUFFER_TYPE	9

				/* commands */
#define COMMAND_ACCEPTED	1
#define COMMAND_REJECTED	2
#define DATAPACKAGE		3
#define CHECK_IF_PRESENT	4
#define CONTROL_GOODBYE		5
#define ANA_HERE_I_AM		10
#define ANA_NEW_STATUS		11
#define ANA_GOODBYE		12
#define ANA_FORCED_DELETE	13
#define CTR_ANALIST		20
#define GET_ITEM		30
#define HISTOGRAM_HEADER	31
#define HISTOGRAM_TRAILER	32
#define PICTURE_HEADER		33
#define PICTURE_TRAILER		34
#define LIST_ITEM		35
#define LIST_TRAILER		36
#define HISTOGRAM_INFO		37
#define ZONE_INFO		38
#define ANA_STOP		40
#define ANA_START		41
#define ANA_ABORT		42
#define ANA_STATUS		43
#define ANA_SAVE		44
#define ANA_LIST_H		45
#define ANA_LIST_P		46
#define CLEAR_ALL		47
#define CLEAR_SINGLE		48
#define ANA_PASSWORD		49
#define SAVE_IN_PROGRESS	50
#define STOP_BUFFER		51
#define ANA_EXTENSION		52
#define ANA_LOAD		53

				/* buffer modes */
#define WAIT_FOR_MORE	1
#define DATA_COMPLETE	2

				/* tcp errors */
#define TCP_OK		   0
#define TCP_ERROR	  -1
#define TCP_TIMEOUT	 101
#define TCP_INTERNAL	 102
#define TCP_UNKNOWN_HOST 103
#define TCP_NODTP	 104
#define TCP_NOACP	 105

			/* analyses list */
typedef struct {
        char    name[20];
        char    host[100];
        int	port;
	int	status;
	int	no_check;
        char	owner[20];
        } analist;

			/* initfile list */
typedef struct {
	char	section[40];
	char	initfile[100];
	} filelist;

			/* analysis data */
typedef struct {
	char	sv_host[80];
	int	sv_port;
	char	a_name[40];
	char	a_password[40];
	int	a_port;
	int	a_socket;
	int	a_status;
	int	a_sleeptime;
	char	a_initfile[100];
	char	a_source[20];
	char	a_dest[20];
	fd_set  a_active;
	int	a_max;
	int	a_save;
	char	saveset[100];
	int	buffer;
        char    a_owner[20];
        } maindef;

			/* histogram arrays */
typedef struct {
	char    basename[30];
	int	lowx;
	int	highx;
        int     baseid;
        } hist_array;

typedef struct {
	char    basename[30];
	int	lowx;
	int	highx;
        } hist_array_old;

			/* single histograms ab V2.7B */
typedef struct {
        char    histname[40];
        int     id;
        char    x_axis[40];
        char    y_axis[40];
        char    title[60];
	int	nx;
	float	xmin;
	float	xmax;
	int	ny;
	float	ymin;
	float   ymax;
	float	packing;
        int	flags;
	} histo;

			/* single histograms bis V2.6E */
typedef struct {
        char    histname[40];
        int     id;
        char    x_axis[40];
        char    y_axis[40];
        char    title[60];
	int	nx;
	float	xmin;
	float	xmax;
	int	ny;
	float	ymin;
	float   ymax;
	float	packing;
	} histo_v2_6E;

#define	HF_PROFILE	(1<<0)		/* Use Profile Histogram */
#define HF_ERROR	(1<<1)		/* Use Errors */

			/* pictures */
typedef struct {
	char	pictname[40];
        char    title[60];
        float   size_x;
        float   size_y;
        int     zone_x;
        int     zone_y;
	int	hnum;
        char    histline[32][80];
        char    zoneinfo[32][20];
        } pict;

			/* control buffer for tcpip */
typedef struct {
	int	header[HEADER_SIZE];
	union {
	  char	c[CBUFFER_SIZE];
	  short s[SBUFFER_SIZE];
	  int	i[IBUFFER_SIZE];
	  } buffer ;
	} control_buffer;

extern maindef 		ana_def;
extern int   		requested_pawspace;
extern int		current_id;
extern histo 		hlist[MAX_HISTO];
extern int   		hcount;
extern pict  		plist[MAX_PICTURE];
extern int   		pcount;
extern hist_array 	alist[MAX_ARRAY];
extern int   		acount;
extern int 		tcp_timeout;		
extern int 		tcp_errno;		

extern void*		newfile[MAX_SECTION];
extern short		newfile_pointer;
extern void*		extension[MAX_SECTION];
extern short		extension_pointer;
extern void*		newfile[MAX_SECTION];
extern short		newfile_pointer;

#ifdef WARPFILL
extern int		id_addr[4*MAX_HISTO];
#endif

void   ana_main(int av, char** ac);
void   get_initfile(char* initfile, char* section);
int    add_to_newfile(void (*dummy)(char* filename));
int    add_to_extension(int (*dummy)(int socket, control_buffer* cb));
int    add_to_endana(void (*dummy)());
int    add_to_calibration(void (*dummy)());
void   process_newfile(char* filename);
int    process_extension(int socket, control_buffer* cb);
void   process_endana();
void   process_docal();
int    look_for_requests();
int    ana_stop();
void   analyze(short* buffer);
int    assign_id_new( char* name, int flag);
int    assign_array( char* name, int* array);
void   enable_error( int id );
int    read_h_entry( char* name, FILE* fp);
int    read_p_entry( char* name, FILE* fp);
histo* get_h_entry( char* name);
pict*  get_p_entry( char* name);
int    get_h_list( char* name, histo** hl);
int    get_p_list( char* name, pict** pl);
int    save_nid_structure( char* filename);
int    load_nid_structure( char* filename);
void   nid_reset_all();
int    netwrite(int socket, char* buffer, int length, int size);
int    netread(int socket, char* buffer, int length, int size);
void   htons_a(short* buffer, int length);
void   ntohs_a(short* buffer, int length);
void   htonl_a(int* buffer, int length);
void   ntohl_a(int* buffer, int length);
int    install_listen(int port);
int    do_connect(char* host, int port);
struct sockaddr_in host2addr(char* host, int port);
char*  addr2host(struct sockaddr_in* address, int* port);
int    write_buffer(int socket,control_buffer* cb);
int    read_buffer(int socket,control_buffer* cb);
int    write_datapackage(int socket, char* buffer, int length, int size);
int    read_datapackage(int socket, char* buffer, int max_length);
int    send_single_command(int socket, int command);
int    send_message(int socket, int command, char* message);
control_buffer 
       send_command(int socket, int command);
int    send_int(int socket, int value);
int    get_int(int socket);
int    wait_for_socket_r(int socket, int seconds);
int    wait_for_socket_w(int socket, int seconds);
float  get_random(float lower, float upper);

#define UNDIG			get_random(-0.5,0.5)
#define assign_id(A1)		assign_id_new(A1,1)
#define wait_for_socket(A,B) 	wait_for_socket_r(A,B)
#define send_answer(A,B)     	send_message(A,B,NULL)
#define set_tcp_timeout(A)   	(tcp_timeout = A)

#if defined(FASTFILL)
# define NHFILL(A,X,Y,W)	{ if (A) HFILL(A,X,Y,W); }
# define NHFILL1(A,X,Y,W)	{ if (A) HF1(A,X,W); }
# define NHFILL2(A,X,Y,W)       { if (A) HF2(A,X,Y,W); }
#elif defined(WARPFILL)
# define NHFILL(A,X,Y,W)	{ if (A) HFILL(A,X,Y,W); }
# define NHFILL1(A,X,Y,W)	{ if (A) HFF1(A,id_addr[A+2*MAX_HISTO],X,W); }
# define NHFILL2(A,X,Y,W)	{ if (A) HFF2(A,id_addr[A+2*MAX_HISTO],X,Y,W); }
#else
# define NHFILL(A,X,Y,W)	{ if (A) HFILL(A,X,Y,W); }
# define NHFILL1(A,X,Y,W)	{ if (A) HFILL(A,X,Y,W); }
# define NHFILL2(A,X,Y,W)	{ if (A) HFILL(A,X,Y,W); }
#endif

					/* some global constants */

#define C_LIGHT         29.98		/* Speed of light (cm/ns) */
#define M_PROTON        938.27		/* Proton mass (MeV) */
#define M2_PROTON       880350.59
#define M_NEUTRON       939.57		/* Neutron mass (Mev) */
#define M2_NEUTRON      882791.78
#define M_DEUTERON      1875.61		/* Deutron mass (MeV) */
#define M_PION          134.97		/* Pi 0 mass (MeV) */
#define M2_PION         18216.90	
#define M_ETA           547.45		/* Eta mass (MeV) */
#define M2_ETA          299701.5
#define M_ELECTRON      0.511           /* Electron mass (MeV) */
#define M2_ELECTRON     0.261121

#define DTORAD          0.01745329	/* degree * DTORAD = rad */

#if defined(SQR)
#  undef SQR
#  define SQR(x) ((x)*(x))
#endif
				/* readout modes in unpack_event.c */

#define UNCOMPRESSED    0       /* Raw Data (Mainz 1995) */
#define COMPRESSED      1       /* Compressed Data (Mainz Level 10) */
#define GEANT           2       /* Simulation Output */

extern int readout_mode;

				       /* Main variables for the
					  analysis */

extern unsigned int	control_flags;
extern int		DEBUG;

				       /* control flags used in
					  library functions: */

#define CALIBRATE               (1<<0) /* Call calibration function */

#define FILL_BAFS_ON_CFD	(1<<1) /* Fill Mode for BaFs */
#define FILL_BAFS_ON_LED	(1<<2)
#define FILL_BAFS_ON_VETO	(1<<3)

#define FRAGMENTATION		(1<<4) /* Process fragmented events */



void do_calibration();		       /* Calibration */
int  unpack_event(short*);	       /* Unpack Event */
void fill_standard_spectra();	       /* Fill Standard Spectra */
void fill_scaler_spectra();	       /* Fill Scaler Spectra */


int pack_data();		       /* Compression */
int pack_scaler();
void write_buffers();
void reset_buffer();

/* Cut definitions */

#define MAX_CUTDEF	20
#define MAX_CUTDIM	50

typedef struct {
	float x[MAX_CUTDIM];
  	float y[MAX_CUTDIM];
  	int   dim;
} cutdef_proto;

extern cutdef_proto cutdef[MAX_CUTDEF];

#endif /* __ASLHEADER */



