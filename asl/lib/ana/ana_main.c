/* Analyses Support Library
 * Main Section
 *
 * The ana_main() function should be called by the user in his/her
 * main program
 *
 * Revision date	Reason
 * --------------------------------------------------------------------------
 * 27.04.1995	VH	Password feature, sleeping mode
 * 02.05.1995	VH	Status word changed (now bits on/off)
 * 09.06.1995   VH	Server registration clean
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "init_cfortran.h"
#include "asl.h"

#if defined(OSF1) || defined(ULTRIX)
# include <strings.h>
# include <sys/types.h>
# include <pwd.h>
#elif defined(VMS)
# include <string.h>
# include <types.h>
#endif

extern int file_open;		       /* from get_buffer.c */

filelist	initfl[MAX_SECTION];
int		initfl_pointer;
maindef		ana_def;

unsigned int	control_flags;		

void*  newfile[MAX_SECTION];
short newfile_pointer = 0;
void*  extension[MAX_SECTION];
short extension_pointer = 0;
void*  endana[MAX_SECTION];
short endana_pointer = 0;
void*  docal[MAX_SECTION];
short docal_pointer = 0;

void  (*exec_newfile)(char* filename);
int   (*exec_extension)(int socket, control_buffer* cb);
void  (*exec_endana)();
void  (*exec_docal)();

void print_help();
int  server_hallo();
int  server_goodbye();
void ana_loop();
int  ana_init();
int  ana_book();
void end_get_buffer();
int  ana_stop();
int  ana_save();

/*****************************************************************************
 *
 *  Example of initfile:
 *
 * SERVER_HOST	localhost 	  # default
 * SERVER_PORT	9011
 * ANA_NAME	Lisa
 * ANA_PASSWORD	taps 		  #default
 * ANA_STATUS	START		  # CLEAR, ABORT, SECURE, STABLE
 * ANA_SOURCE	TCPIP
 * ANA_SAVESET  last.rzdat
 * ANA_SAVE	1000              # secs for backup
 * HISTFILE	deuterium_tape.rzdat
 *
 * INITFILE	MAIN			initfile.par
 * INITFILE	GET_BUFFER		initfile.par
 * INITFILE	UNPACK_EVENT		unpack_event.par
 * ...
 *
 ****************************************************************************/

void ana_main(int argc, char** argv)
{
  int	current_time;
  int 	i;
  FILE*	fp;
  char*	loop;
  char	line[200];
  char	word[40], arg1[100], arg2[100];
  char	histfile[100], nidfile[100];
//  extern short do_end_sane;
  short do_end_sane;
  extern int n_scaler_events;
  extern float realtime_ext;
  extern float scaler_ext[353];
#if defined(OSF1) || defined(ULTRIX)
  struct passwd *pwd;
#endif

  control_flags = 0;
  n_scaler_events = 0;
  realtime_ext = 0;
  do_end_sane = 0;

  for ( i = 1; i <= 352; i++) {
	  scaler_ext[i] = 0;
  }
  
  current_time = time(NULL);
  printf("*** %s ***\n",ASL_VERSION);
  printf("\n");
  printf("*** ANALYSES STARTUP SEQUENCE ***\n");
  printf("\n");
  printf("<I> Today is %s",ctime(&current_time));
  printf("\n");


			/* set defaults */
  strcpy(ana_def.sv_host,"localhost");
  ana_def.sv_port = 9011;
  strcpy(ana_def.a_name,"test");
  strcpy(ana_def.a_password,"taps");
#if defined(OSF1) || defined(ULTRIX)
  strcpy(ana_def.a_initfile,"./initfile.par");
  strcpy(ana_def.saveset,"./last.rzdat");
  pwd = getpwuid(getuid());
  strcpy(ana_def.a_owner,pwd->pw_name);
#elif defined(VMS)
  strcpy(ana_def.a_initfile,"initfile.par");
  strcpy(ana_def.saveset,"last.rzdat");
  ana_def.a_owner[0] = 0;
#endif
  strcpy(ana_def.a_source,"NONE");
  strcpy(ana_def.a_dest,"NONE");
  ana_def.a_save   = 0;
  ana_def.a_status = A_SEEN;
  ana_def.buffer = 0;
  initfl_pointer = 0;
  histfile[0] = 0;
  
			/* check command line */
  printf("<I> Checking command line arguments ...\n");
  if (argc>1) {
    for(i=0;i<argc;i++) {
      if (!strcmp("-i",argv[i])) {
	strcpy(ana_def.a_initfile,argv[i+1]);
	printf("    initfile set to %s\n",ana_def.a_initfile);
      }
      else if (!strcmp("-I",argv[i])) {
	strcpy(ana_def.a_initfile,argv[i+1]);
	printf("    initfile set to %s\n",ana_def.a_initfile);
      }
      else if (!strcmp("-h",argv[i]))
	print_help();
      else if (!strcmp("-H",argv[i]))
	print_help();
      else if (!strcmp("-?",argv[i]))
	print_help();
    }
  }

			/* read initfile */
  printf("<I> Reading main initfile ...\n");
  if ((fp=fopen(ana_def.a_initfile,"r"))==NULL) {
    perror("<E> Cannot open initfile");
    exit(1);
  }
  
  while (fgets(line, sizeof(line), fp)) {
    if (sscanf(line,"%s %s %s",word,arg1,arg2)>0) {
      if (!strcmp(word,"SERVER_HOST")) 
	strcpy(ana_def.sv_host,arg1);
      else if (!strcmp(word,"SERVER_PORT"))
	ana_def.sv_port = atoi(arg1);
      else if (!strcmp(word,"ANA_NAME"))
	strcpy(ana_def.a_name,arg1);
      else if (!strcmp(word,"ANA_PASSWORD"))
	strcpy(ana_def.a_password,arg1);
      else if (!strcmp(word,"ANA_STATUS"))  {
        if (!strcmp(arg1,"CLEAR"))	 ana_def.a_status  = A_SEEN;
        else if (!strcmp(arg1,"START"))  ana_def.a_status |= A_RUNNING;
        else if (!strcmp(arg1,"ABORT"))  ana_def.a_status |= A_AUTOABORT;
        else if (!strcmp(arg1,"SECURE")) ana_def.a_status |= A_SECURE;
        else if (!strcmp(arg1,"STABLE")) ana_def.a_status |= A_STABLE;
        else if (!strcmp(arg1,"UNSEEN")) ana_def.a_status &= (~A_SEEN);
      }
      else if (!strcmp(word,"ANA_SOURCE"))
	strcpy(ana_def.a_source,arg1);
      else if (!strcmp(word,"ANA_DEST"))
	strcpy(ana_def.a_dest,arg1);
      else if (!strcmp(word,"ANA_SAVE"))
	ana_def.a_save = atoi(arg1);
      else if (!strcmp(word,"ANA_SAVESET"))
	strcpy(ana_def.saveset,arg1);  
      else if (!strcmp(word,"HISTFILE"))
	strcpy(histfile,arg1);
      else if (!strcmp(word,"INITFILE")) {
	strcpy(initfl[initfl_pointer].section,arg1);
	strcpy(initfl[initfl_pointer].initfile,arg2);
	initfl_pointer++;
      }
    }
  }
  
  printf("<I> Initial settings fixed to ...\n");
  printf("    Initfile  : %s\n",ana_def.a_initfile);
  printf("    Server    : %s\n",ana_def.sv_host);
  printf("    Port      : %d\n",ana_def.sv_port);
  printf("    Name      : %s\n",ana_def.a_name);
  printf("    Password  : %s\n",ana_def.a_password);
  printf("    Status    : %x\n",ana_def.a_status);
  printf("    Saveset   : %s\n",ana_def.saveset);
  printf("    Savetime  : %d s\n",ana_def.a_save);
  if (histfile[0]) 
    printf("    Histfile  : %s\n",histfile);
  printf("    Initfiles :\n");
  for(i=0;i<initfl_pointer;i++) 
    printf("      %-25s : %s\n",initfl[i].section,initfl[i].initfile);
  
  if (ana_def.a_status & A_SEEN)  {
    printf("<I> Connecting to server ...\n");
    if ((ana_def.a_port = server_hallo())<0) {
      perror("<E> Cannot register at server");
      exit(1);
    }
  }
  else
    printf("<W> Analyses not registered due to DATA CARD.\n");
  
  FD_ZERO(&ana_def.a_active);
  printf("<I> Install listen at port %d\n",ana_def.a_port);
  if ((ana_def.a_socket = install_listen(ana_def.a_port))<0) {
    perror("<E> Error setting listen mode");
    if (ana_def.a_status & A_SEEN)  server_goodbye();
    exit(1);
  }
  FD_SET(ana_def.a_socket, &ana_def.a_active);
  ana_def.a_max = ana_def.a_socket + 1;
  
  printf("<I> Processing initfiles:\n");
  if ((i=ana_init())<0) {
    fprintf(stderr,"<E> Error doing init\n");
    if (ana_def.a_status & A_SEEN)  server_goodbye();
    exit(1);
  }
  printf("    Done.\n");
  
  if (histfile[0]) {
    printf("<I> Histogram definitions from parameterfiles not used !\n");
    printf("    Loading histogram definitions from file\n");
#if defined (VMS)
    sprintf(nidfile,"%s_hlist",histfile);
#else
    sprintf(nidfile,"%s.hlist",histfile);
#endif
    if ( load_nid_structure(nidfile)<0) 
      printf("<W> Cannot read definition file\n");
  }
  
  
  printf("<I> Booking histograms and pictures\n"); 
  if (ana_book()<0) {
    perror("<E> Error booking histogram or picture\n");
    if (ana_def.a_status & A_SEEN)  server_goodbye();
    exit(1);
  }

  if (histfile[0]) {
    printf("    Loading histogram entries from file\n");
    HCDIR("//PAWC"," ");
    HRGET(0,histfile,"A");
  }

  printf("    Done.\n");
  
  printf("\n*** Entering main loop ...\n\n");
  ana_loop();
  
  current_time = time(NULL);
  printf("\n*** ANALYSES SHUTDOWN SEQUENCE ***\n");
  printf("\n");
  printf("<I> Today is %s\n",ctime(&current_time));
  printf("\n");
  printf("<I> Number of Scaler Events is %6d\n", n_scaler_events);
  printf("\n");
  printf("  Last Scaler Extern Dump:\n");
  for ( i = 1; i <= 352; i++) {
	  printf( "  %3d  %8.0f\n", i, scaler_ext[i]);
  }
  printf("\n");

  process_endana();

  /*
  printf("<I> Stop getting buffer\n");
  end_get_buffer();

  printf("<I> Stop writing buffer\n");
  end_put_buffer();
  */ /* Now done via the endana-stack ! */
  
  printf("<I> Disable listen\n");
  close(ana_def.a_socket);
  
  printf("<I> Deleting from server\n");
  if (ana_def.a_status & A_SEEN)  
    if (server_goodbye()<0) perror("<E> Cannot delete entry at server");
  
//  end_sane();

  printf("<I> Saving all histograms to file %s\n", ana_def.saveset);
  if (ana_save(NULL)<0) perror("<E> Cannot save histograms\n"); 
  return;
}

/*****************************************************************************/

void print_help()
{

fprintf(stderr,"\n");
fprintf(stderr,"    ANALYSES STARTUP COMMAND\n");
fprintf(stderr,"\n");
fprintf(stderr,"    -i file  initfile\n");
fprintf(stderr,"    -h       help\n");
fprintf(stderr,"\n");

exit(0);
}

/*****************************************************************************/

int server_hallo()
{
int sd;
int port;
control_buffer cb;
struct sockaddr_in sa;
int length;

if ((sd=do_connect(ana_def.sv_host,ana_def.sv_port))<0)
	return -1;

length = sizeof(sa);
if (getsockname(sd,(struct sockaddr*) &sa,&length)<0)
	return -10;

if (addr2host(&sa,&port)==NULL)
	return -11;

sprintf(cb.buffer.c,"%d %s %s",ana_def.a_status, ana_def.a_name,
	                       ana_def.a_owner);

cb.header[COMMAND] = ANA_HERE_I_AM;
cb.header[BUFFER_TYPE] = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;

if (write_buffer(sd,&cb)<0) 
	return -2;

if (read_buffer(sd,&cb)<0)
	return -3;

if (cb.header[COMMAND] != COMMAND_ACCEPTED)
	return -4;

printf("    Analyses %s on port %d registered\n",
				ana_def.a_name, port);

close(sd);
return port;
}

/*****************************************************************************/

int server_goodbye()
{
int sd;
control_buffer cb;

if ((sd=do_connect(ana_def.sv_host,ana_def.sv_port))<0)
	return -1;

sprintf(cb.buffer.c,"%d %s %d",ana_def.a_status, ana_def.a_name,
						 ana_def.a_port);

cb.header[COMMAND] = ANA_GOODBYE;
cb.header[BUFFER_TYPE] = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;

if (write_buffer(sd,&cb)<0) 
	return -2;

if (read_buffer(sd,&cb)<0)
	return -3;

if (cb.header[COMMAND] != COMMAND_ACCEPTED)
	return -4;

printf("    Analyses %s on port %d deleted\n",
				ana_def.a_name, ana_def.a_port);

close(sd);
return 0;
}

/*****************************************************************************/

void ana_loop()
{
	int current_time = 0;
	int new_time = 0;
	int loop = 1;
	int status;
	short buffer[SBUFFER_SIZE];

	current_time = time(NULL);
	new_time = current_time;
  
	// analysis loop until ABORT command
	do {
		// look for requests
		look_for_requests();

		// do analysis
		if (ana_def.a_status & A_SLEEPING) {
			if (ana_def.a_sleeptime > 0) {
				ana_def.a_sleeptime--;
				sleep( 1);
			}
			else  ana_def.a_status &= (~A_SLEEPING);
		}
		else if (ana_def.a_status & A_RUNNING) {
			status = do_get_buffer( buffer);
			if (status == 0) analyze( buffer);
			else if (status == -1) ana_stop();
		}  
		else if (ana_def.a_status & A_ABORTED) loop = 0;
		else if ( (ana_def.a_status & A_AUTOABORT) && (!file_open) ) loop = 0;

		if (ana_def.a_save) new_time = time( NULL);

		if ( new_time > (current_time+ana_def.a_save) )  {
			ana_save( NULL);
			current_time = new_time;
		}
  
	} while ( loop);

	return;
}

/*****************************************************************************/

void get_initfile(char* inf,char* section)
{
int     i;

inf[0]=0;
for(i=0;i<initfl_pointer;i++)
  if (!strcmp(initfl[i].section,section))
    strcpy(inf, initfl[i].initfile);
return;
}

/*****************************************************************************/

int add_to_newfile(void (*dummy)(char* filename))
{
  if ( (newfile_pointer+1) < MAX_SECTION ) {
    newfile[++newfile_pointer] = (void*) dummy;
    return 0;
  }
  else {
    return -1;
  }
}

/*****************************************************************************/

void process_newfile(char* filename)
{
  if (newfile_pointer) {  
    int i;
    for (i=1;i<=newfile_pointer;i++) {
      exec_newfile = (void (*)(char*))(newfile[i]);
      exec_newfile(filename);
    }
  }
}

/*****************************************************************************/

int add_to_extension(int (*dummy)(int socket, control_buffer* cb))
{
  if ( (extension_pointer+1) < MAX_SECTION ) {
    extension[++extension_pointer] = (void*) dummy;
    return 0;
  }
  else {
    return -1;
  }
}

/*****************************************************************************/

int process_extension(int socket, control_buffer* cb)
{
  int status;
  
  if (extension_pointer) {  
    int i;
    for (i=1;i<=extension_pointer;i++) {
      exec_extension = (int (*)(int, control_buffer*))(extension[i]);
      status = exec_extension(socket,cb);
      if (status == 1) return 1;
    }
  }
  if (status) return 2;
  else        return 0;
}

/*****************************************************************************/

int add_to_endana(void (*dummy)())
{
  if ( (endana_pointer+1) < MAX_SECTION ) {
    endana[++endana_pointer] = (void*) dummy;
    return 0;
  }
  else {
    return -1;
  }
}

/*****************************************************************************/

void process_endana()
{
  if (endana_pointer) {  
    int i;
    for (i=1;i<=endana_pointer;i++) {
      exec_endana = (void (*)())(endana[i]);
      exec_endana();
    }
  }
}

/*****************************************************************************/

int add_to_calibration(void (*dummy)())
{
  if ( (docal_pointer+1) < MAX_SECTION ) {
    docal[++docal_pointer] = (void*) dummy;
    return 0;
  }
  else {
    return -1;
  }
}

/*****************************************************************************/

void process_docal()
{
	if ( docal_pointer) {  
		int i;
		for ( i = 1; i <= docal_pointer; i++) {
			exec_docal = (void (*)())(docal[i]);
			exec_docal();
		}
	}
}

/*****************************************************************************/

/* Analyses Support Library
 * Booking histograms
 *
 * This module calls the user function book_user() and
 * books the internal histograms of get_buffer().
 *
 */
 

int book_user();
int book_calibration();
int book_unpack_event();
int book_pack_event();
int book_standard_spectra();

int ana_book()
{

if (book_user()<0)	 	return -1;
if (book_calibration()<0) 	return -1;
if (book_unpack_event()<0)	return -1;
if (book_pack_event()<0)	return -1;
if (book_standard_spectra()<0)	return -1;

printf("    Requested HBOOK Data Space = %d\n",requested_pawspace);  
  
return 0;
}

/*****************************************************************************/

/* Analyses Support Library
 * Initialization Phase
 *
 * This modules calls the init section of get_buffer() and
 * the user function init_user()
 *
 */


int init_get_buffer();
/* int init_put_buffer(); */
int init_geometry();
int init_calibration();
int init_unpack_event();
int init_pack_event();
int init_standard_spectra();
int init_user();

int ana_init()
{

if (init_get_buffer()<0)
    return -1;
/* if (init_put_buffer()<0)
    return -1; */
if (init_calibration()<0) 	/* init_calibration must be called */
    return -1;			/* before init_geometry */
if (init_geometry()<0)
    return -1;
if (init_unpack_event()<0)
    return -1;
if (init_pack_event()<0)
    return -1;
if (init_standard_spectra()<0)
    return -1;

return init_user();

}
