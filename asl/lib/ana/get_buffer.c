/* Analysis Support Library
 * Getting Buffer
 *
 * This modules reads buffers from several sources and offer
 * them to the user.
 *
 * Revision date	Reason
 * --------------------------------------------------------------------------
 * 27.04.1995	VH	Adding TCP-Request on connect,
 *			TCPIP retry feature
 * 10.07.1995	VH	Bug Fix: Reset timeout counter, if successful
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "init_cfortran.h"
#include "asl.h"

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

#define	SECTION	"GET_BUFFER"

#define S_TCPIP		1
#define S_FILE		2
#define S_FILELIST	3
#if defined(OSF1) || defined(ULTRIX)
# define S_TAPE		4
# define S_TAPELIST	5
# define MAX_TAPE_BUFFER_SIZE 24576
#endif

#define TCP_REQUEST_MODE 3
#define TCP_CONTINOUS_MODE 4
#define TCP_BUFFER_REQUEST 5
#define TCP_STOP 6

int source;
int byteorder;
int wordorder;

char buffer_server[100];			// tcpip
int buffer_port;
int buffer_socket;
int tcp_error_continue = -1;
int tcp_error_wait     = 30;
int tcp_timeout_retry  = 0;

char listname[100];				// filelist
FILE* listpointer;

char filename[100];				// file
extern char dfilename[100];
FILE* filepointer;

short run_char;		// This stuff is for run id
char run_num;
char *s1, *s2; 
extern short veto5_flag, tape_flag;

static short file_count;

int	file_has_changed = 1;
int	file_open;

#if defined(OSF1) || defined(ULTRIX)
 char		tapename[100];
 ANSI_TAPE*	tape;
 TAPE_ENTRY	entry;
 short		tape_buffer_size = 1;
 short		tape_buffer[MAX_TAPE_BUFFER_SIZE];
 short		tape_buffer_pointer = 0;
 short		initial_file_skip = 0;

#define MAX_TAPEFILES 100 
				       /* contains filenames to analyze and
					  to ignore */
 char		tape_filelist[MAX_TAPEFILES][20];
 int		tape_fileaction[MAX_TAPEFILES];
 short		tape_filepointer;

#define		TF_ACCEPT	1
#define		TF_IGNORE	2
#define		TF_END		3
#define		TF_STOP		4

#endif

int 	extension_get_buffer(int, control_buffer*);
short*  current_buffer;

void end_get_buffer();

/****************************************************************************/
/*

Description of Initfile with default values 

BUFFER_SERVER	e6tap3		#default
BUFFER_PORT	7584		#default
BUFFER_RETRY	0		# Flag 1
ERROR_WAIT	30		# secs

TAPE		/dev/rmt0h
TAPEBUFFER	3
SKIP		0

FILE		./data.lmd

LIST		./data.list

BYTEORDER	0
WORDORDER	0

*/
/****************************************************************************/

int init_get_buffer()
{
int 	i;
char	initfile[100];
FILE*	fp;
char	line[120];
char	key[40], arg[80];
int	netstat;
short	buffer[SBUFFER_SIZE];
					/* Status */
printf("<I> Init section: %s\n",SECTION);

file_count = 0;

					/* get initfile */
get_initfile(initfile,SECTION);
if (initfile[0] == 0) {
	fprintf(stderr,"    No initfile for this section found\n");
	return -1;
	}

					/* set defaults */
strcpy(buffer_server,"e6tap3");
buffer_port = 7584;
#if defined(OSF1) || defined(ULTRIX)
 strcpy(tapename,"/dev/rmt0h");
#endif
strcpy(filename,"./data.lmd");

strcpy(listname,"./data.list");
byteorder = 0;
wordorder = 0;
					/* open initfile */
if ((fp=fopen(initfile,"r"))==NULL) {
	perror("<E> Cannot open initfile");
	fprintf(stderr,"    File = %s\n",initfile);
	return -1;
	}

					/* read initfile */
while (fgets( line, sizeof(line), fp)) {
  if (sscanf(line,"%s %s",key,arg)>0) {
    if (!strcmp(key,"BUFFER_SERVER")) 
	strcpy(buffer_server,arg);
    else if (!strcmp(key,"BUFFER_PORT"))
	buffer_port = atoi(arg);
    else if (!strcmp(key,"BUFFER_RETRY")) 
	if (!strcmp(arg,"0"))	tcp_error_continue = -1;
	else			tcp_error_continue =  1;
    else if (!strcmp(key,"ERROR_WAIT"))
	tcp_error_wait = atoi(arg);
    else if (!strcmp(key,"BYTEORDER"))
	byteorder = atoi(arg);
    else if (!strcmp(key,"WORDORDER"))
	wordorder = atoi(arg);
#if defined(OSF1) || defined(ULTRIX)
    else if (!strcmp(key,"TAPEBUFFER"))
	tape_buffer_size = atoi(arg);
    else if (!strcmp(key,"TAPE"))
	strcpy(tapename,arg);
    else if (!strcmp(key,"SKIP"))
	initial_file_skip = atoi(arg);
#endif
    else if (!strcmp(key,"FILE"))
	strcpy(filename,arg);
    else if (!strcmp(key,"LIST"))
	strcpy(listname,arg);
    else if (!strcmp(key,"HISTOGRAM"))
	read_h_entry(arg,fp);
    else if (!strcmp(key,"PICTURE"))
	read_h_entry(arg,fp);
    }
  }

fclose(fp);

					/* react on source */
if (!strcmp(ana_def.a_source,"TCPIP")) {
  if ((buffer_socket=do_connect(buffer_server,buffer_port))<0) {
	perror("<E> Cannot connect to buffer server");
  	fprintf(stderr,"    Host: %s Port: %d\n",buffer_server, buffer_port);
	return -1;
	}
  if (send_int(buffer_socket,TCP_REQUEST_MODE)<0) {
	perror("<E> Cannot set TCPIP buffer mode");
  	fprintf(stderr,"    Host: %s Port: %d\n",buffer_server, buffer_port);
	close(buffer_socket);
	return -1;
	}
  printf("    Getting buffer via TCPIP\n");
  printf("    Host: %s Port: %d\n",buffer_server, buffer_port);
  file_open = 1;
  source = S_TCPIP;
  }

#if defined(OSF1) || defined(ULTRIX)
 else if (!strcmp(ana_def.a_source,"TAPE")) {
  if ((tape=t_mount(tapename))==NULL) {
	perror("<E> Cannot mount tape");
	fprintf(stderr,"    Tape = %s\n",tapename);
	return -1;
	}
  printf("    Tape %s mounted\n",tape->label);
  printf("    Reading %d buffers per block\n",tape_buffer_size);
  tape->status |= T_USE_HEADER;
  if (initial_file_skip) {
    printf("    Initial File Skip: %d\n",initial_file_skip);
    t_goto(tape,initial_file_skip);
  }
  file_open = 0;
  source = S_TAPE;  
  }

 else if (!strcmp(ana_def.a_source,"TAPELIST")) {
   if ((tape=t_mount(tapename))==NULL) {
     perror("<E> Cannot mount tape");
     fprintf(stderr,"    Tape = %s\n",tapename);
     return -1;
   }
   printf("    Tape %s mounted\n",tape->label);
   printf("    Reading %d buffers per block\n",tape_buffer_size);
   if ((listpointer=fopen(listname,"r"))==NULL) {
     perror("<E> Cannot open listfile");
     fprintf(stderr,"    File = %s\n",listname);
     return -1;
   }
   				       /* read listfile in case of TAPE */
   printf("    Reading Listfile %s\n",listname);
   while (fgets( line, sizeof(line), listpointer)) {
     if (sscanf(line,"%s %s",key,arg)>0) {
				       /* check empty line and comment */
       if ( (key[0]!=0) && (key[0]!='#') ) {
	 if (tape_filepointer<MAX_TAPEFILES) {
	   if (!strcmp(arg,"-")) {     /* Ignore */
	     strncpy(tape_filelist[tape_filepointer],key,20);
	     tape_fileaction[tape_filepointer++] = TF_IGNORE;
             printf("    Ignore     : <%s>\n",key);
	   }
	   else if (!strcmp(arg,"STOP")) { /* Stop after file */
	     strncpy(tape_filelist[tape_filepointer],key,20);
	     tape_fileaction[tape_filepointer++] = TF_STOP;
             printf("    Stop before: <%s>\n",key);
	   }
	   else if (!strcmp(arg,"END")) { /* End after file */
	     strncpy(tape_filelist[tape_filepointer],key,20);
	     tape_fileaction[tape_filepointer++] = TF_END;
             printf("    End before : <%s>\n",key);
	   }
	   else {		       /* Accept */
	     strncpy(tape_filelist[tape_filepointer],key,20);
	     tape_fileaction[tape_filepointer++] = TF_ACCEPT;
	     printf("    Accept     : <%s>\n",key);
	   }
	 }
	 else {
	   printf("<W> Increase MAX_TAPEFILES: ");
	   printf("    Pattern %s\n",key);
	 }
       } /* end no empty line or comment */
     } /* end valid line */
   } /* end while */
   fclose(listpointer);		       /* close file */
   listpointer = NULL;
	 
   tape->status |= T_USE_HEADER;
   if (initial_file_skip) {
     printf("    Initial File Skip: %d\n",initial_file_skip);
     t_goto(tape,initial_file_skip);
   }
   file_open = 0;
   source = S_TAPELIST;  
  }
#endif

else if (!strcmp(ana_def.a_source,"FILE")) {
  if ((filepointer=fopen(filename,"rb"))==NULL) {
	perror("<E> Cannot open datafile");
	fprintf(stderr,"    File = %s\n",filename);
	return -1;
	}
  printf("    Datafile %s opened\n",filename);
  process_newfile(filename);
  file_open = 1;
  source = S_FILE;

	if ( (s1 = rindex( filename, '/')) != NULL) s2 = s1 + 5;	// This is for disc files
	else s2 = filename + 4;								// This is for tape files
	s2[3] = '\0';
	run_char = s2[2];
	if ( run_char < 97) run_char += 32;					// This converts capital letters
	s2[2] = '\0';										// to small ones
	run_num = atoi( s2);
	printf( "    Run ID is: DPI %2d - %c\n", run_num, run_char);
	printf( "    File Counter = %3d\n", ++file_count);

	if ( run_num < 7) veto5_flag = 0;						// Runs before "dpi_07f" should use
	else if ( run_num > 7) veto5_flag = 1;					// the normal veto 5 position.  The
	else if ( run_char <= 101) veto5_flag = 0;				// rest should use the newer position.
	else if ( run_char > 101) veto5_flag = 1;				// See "unpack_event.c."

//	if ( veto5_flag == 0) printf( "    Using setno = 54 for Veto #5\n");
//	else if ( veto5_flag == 1) printf( "    Using setno = 63 for Veto #5\n");

	if ( run_num < 8) tape_flag = 1;
	else if ( run_num < 14) tape_flag = 2;
	else tape_flag = 3;
//	printf( "    Tape Number is %d\n", tape_flag);

	strcpy( dfilename, filename);
  }

else if (!strcmp(ana_def.a_source,"FILELIST")) {
  if ((listpointer=fopen(listname,"r"))==NULL) {
	perror("<E> Cannot open listfile");
	fprintf(stderr,"    File = %s\n",listname);
	return -1;
	}
  printf("    Listfile %s opened\n",listname);
  file_open = 0;
  source = S_FILELIST;  
  }

else if (!strcmp(ana_def.a_source,"NONE")) {
  printf("<I> No input of buffers\n");
  source = 0;
}

add_to_extension(extension_get_buffer);  
add_to_endana(end_get_buffer);
  
return;

}

/****************************************************************************/

int do_get_buffer(short* buffer)
{
int	i;
char	line[120];
char    help[40];
char	input;
int	netstat;
static  int try_again = 0;

current_buffer = buffer;  
  
if (!source) return 0;  
  
switch (source) {
  case S_TCPIP :
	if (!file_open) {
 	  if ((buffer_socket=do_connect(buffer_server,buffer_port))<0) {
	    perror("<E> Cannot connect to buffer server");
  	    fprintf(stderr,"    Host: %s Port: %d\n",buffer_server, buffer_port);
	    fprintf(stderr,"	Waiting %d seconds ...\n",tcp_error_wait);
	    ana_def.a_status    |= A_SLEEPING;
	    ana_def.a_sleeptime = tcp_error_wait; 
	    return tcp_error_continue;
	    }
  	  if (send_int(buffer_socket,TCP_REQUEST_MODE)<0) {
	    perror("<E> Cannot set TCPIP buffer mode\n");
  	    fprintf(stderr,"    Host: %s Port: %d\n",buffer_server, buffer_port);
	    close(buffer_socket);
	    return tcp_error_continue;
	    }
          printf("    Getting buffer via TCPIP\n");
          printf("    Host: %s Port: %d\n",buffer_server, buffer_port);
          file_open = 1;
          source = S_TCPIP;
	  tcp_timeout_retry = 0;
          }
	if (!tcp_timeout_retry) {
	  if (send_int(buffer_socket,TCP_BUFFER_REQUEST)<0) {
	    perror("<E> Cannot send TCP buffer request");
	    close(buffer_socket);
	    file_open = 0;
	    return tcp_error_continue;
	  }
	}
	netstat = wait_for_socket_r(buffer_socket,5);
	if (!netstat) {
	  if (tcp_timeout_retry) {
	    printf(".");
	    fflush(stdout);
	    if (!(tcp_timeout_retry % 50))
	      printf("\n    ");
	    tcp_timeout_retry++;
	  }
	  else { 
	    printf("<W> .");
	    fflush(stdout);
       	    tcp_timeout_retry = 1;
	  }
	  return 1;
	}
        else if (netstat<0) {
	  perror("<E> Error waiting for TCPIP buffer");
	  return tcp_error_continue;
	  }	
	if (tcp_timeout_retry) {
	  printf("\n");
          tcp_timeout_retry = 0;	/* reset timeout */
	}
	if (netread(buffer_socket, (char*) buffer, CBUFFER_SIZE, 1)
				< 0) {	
	  perror("<E> Error reading buffer via TCPIP");
	  close(buffer_socket);
	  file_open = 0;
	  return tcp_error_continue;
	  }
	break;
  case S_FILE :
	if (!file_open) {
	  printf("<?> Give new datafile: ");
	  fflush(stdout);
	  scanf("%s",filename);
  	  if ((filepointer=fopen(filename,"rb"))==NULL) {
	    perror("<E> Cannot open datafile");
	    fprintf(stderr,"    File = %s\n",filename);
	    return -1;
	    }
  	  printf("    Datafile %s opened\n",filename);
	  process_newfile(filename);
	  file_has_changed = 1;
  	  file_open = 1;

		if ( (s1 = rindex( filename, '/')) != NULL) s2 = s1 + 5;	// This is for disc files
		else s2 = filename + 4;								// This is for tape files
		s2[3] = '\0';
		run_char = s2[2];
		if ( run_char < 97) run_char += 32;					// This converts capital letters
		s2[2] = '\0';										// to small ones
		run_num = atoi( s2);
		printf( "    Run ID is: DPI %2d - %c\n", run_num, run_char);
		printf( "    File Counter = %3d\n", ++file_count);

		if ( run_num < 7) veto5_flag = 0;						// Runs before "dpi_07f" should use
		else if ( run_num > 7) veto5_flag = 1;					// the normal veto 5 position.  The
		else if ( run_char <= 101) veto5_flag = 0;				// rest should use the newer position.
		else if ( run_char > 101) veto5_flag = 1;				// See "unpack_event.c."

//		if ( veto5_flag == 0) printf( "    Using setno = 54 for Veto #5\n");
//		else if ( veto5_flag == 1) printf( "    Using setno = 63 for Veto #5\n");

		if ( run_num < 8) tape_flag = 1;
		else if ( run_num < 14) tape_flag = 2;
		else tape_flag = 3;
//		printf( "    Tape Number is %d\n", tape_flag);

		strcpy( dfilename, filename);
	  }
	if (fread(buffer,sizeof(char),CBUFFER_SIZE,filepointer)	
				< CBUFFER_SIZE) {	
	  printf("<I> End of file reached\n");
	  fclose(filepointer);
	  file_open = 0;
	  return -1;
	  }
	break;
  case S_FILELIST :
	if (!file_open) {
	  if (!listpointer) {
	    printf("<?> Give new listfile: ");
	    fflush(stdout);
	    scanf("%s",listname);
  	    if ((listpointer=fopen(listname,"r"))==NULL) {
	    	perror("<E> Cannot open listfile");
	    	fprintf(stderr,"    File = %s\n",listname);
	    	return -1;
	    	}
  	    printf("    Listfile %s opened\n",listname);
	    }
	  if (fgets(line, sizeof(line), listpointer)) 
	    sscanf(line,"%s",filename);
	  else {
	    printf("<I> End of listfile reached.\n");
	    fclose(listpointer);
	    listpointer = NULL;
	    return -1;
	    }
  	  if ((filepointer=fopen(filename,"rb"))==NULL) {
	    perror("<E> Cannot open datafile");
	    fprintf(stderr,"    File = %s\n",filename);
	    return -1;
	    }
  	  printf("    Datafile %s opened\n",filename);
	  process_newfile(filename);
	  file_has_changed = 1;
  	  file_open = 1;

		if ( (s1 = rindex( filename, '/')) != NULL) s2 = s1 + 5;	// This is for disc files
		else s2 = filename + 4;								// This is for tape files
		s2[3] = '\0';
		run_char = s2[2];
		if ( run_char < 97) run_char += 32;					// This converts capital letters
		s2[2] = '\0';										// to small ones
		run_num = atoi( s2);
		printf( "    Run ID is: DPI %2d - %c\n", run_num, run_char);
		printf( "    File Counter = %3d\n", ++file_count);

		if ( run_num < 7) veto5_flag = 0;						// Runs before "dpi_07f" should use
		else if ( run_num > 7) veto5_flag = 1;					// the normal veto 5 position.  The
		else if ( run_char <= 101) veto5_flag = 0;				// rest should use the newer position.
		else if ( run_char > 101) veto5_flag = 1;				// See "unpack_event.c."

//		if ( veto5_flag == 0) printf( "    Using setno = 54 for Veto #5\n");
//		else if ( veto5_flag == 1) printf( "    Using setno = 63 for Veto #5\n");

		if ( run_num < 8) tape_flag = 1;
		else if ( run_num < 14) tape_flag = 2;
		else tape_flag = 3;
//		printf( "    Tape Number is %d\n", tape_flag);

		strcpy( dfilename, filename);
	  }
	if (fread(buffer,sizeof(char),CBUFFER_SIZE,filepointer)	
				< CBUFFER_SIZE) {	
	  printf("<I> End of file reached\n");
	  fclose(filepointer);
	  file_open = 0;
	  return 1;
	  }
	break;
#if defined(OSF1) || defined(ULTRIX)
  case S_TAPE :
	if (!tape_buffer_pointer) {
	 if (!tape) {
	  printf("<?> Insert new tape");
	  fflush(stdout);
	  scanf("%c",&input);
	  do {
	    scanf("%c",&input);
	    } while ( (input != 'y') && (input != 'n') );
	  if (input != 'y') {
	    printf("<?> So why did you start ?\n");
	    return -1;
	    } 
  	  if ((tape=t_mount(tapename))==NULL) {
	    perror("<E> Cannot mount tape");
	    fprintf(stderr,"    Tape = %s\n",tapename);
	    return -1;
	    }
  	  printf("    Tape %s mounted\n",tape->label);
  	  tape->status |= T_USE_HEADER;
	  file_open = 0;
	  }
	 if (!file_open) {
	  T_get_next_entry(entry,tape);
	  if (entry.file_sequence_number < 0) {
	    printf("<I> End of tape reached.\n");
	    printf("    Unmounting ... ");
	    fflush(stdout);
	    t_umount(tape);
	    tape = NULL;
	    printf("Done.\n");
	    return -1;
	    }
	  if (t_open(tape,T_READ,NULL)<0) {
	    perror("<E> Error opening tape file, unmounting tape\n");
	    fprintf(stderr,"    File = %s\n",entry.filename);
	    t_umount(tape);
	    tape = NULL;
	    return -1;
	    }
	  printf("<I> New tape file opened: %s\n",entry.filename);
	  process_newfile(entry.filename);
	  file_has_changed = 1;
	  file_open = 1;

		if ( (s1 = rindex( entry.filename, '/')) != NULL) s2 = s1 + 5;	// This is for disc files
		else s2 = entry.filename + 4;								// This is for tape files
		s2[3] = '\0';
		run_char = s2[2];
		if ( run_char < 97) run_char += 32;					// This converts capital letters
		s2[2] = '\0';										// to small ones
		run_num = atoi( s2);
		printf( "    Run ID is: DPI %2d - %c\n", run_num, run_char);
		printf( "    File Counter = %3d\n", ++file_count);

		if ( run_num < 7) veto5_flag = 0;						// Runs before "dpi_07f" should use
		else if ( run_num > 7) veto5_flag = 1;					// the normal veto 5 position.  The
		else if ( run_char <= 101) veto5_flag = 0;				// rest should use the newer position.
		else if ( run_char > 101) veto5_flag = 1;				// See "unpack_event.c."

//		if ( veto5_flag == 0) printf( "    Using setno = 54 for Veto #5\n");
//		else if ( veto5_flag == 1) printf( "    Using setno = 63 for Veto #5\n");

		if ( run_num < 8) tape_flag = 1;
		else if ( run_num < 14) tape_flag = 2;
		else tape_flag = 3;
//		printf( "    Tape Number is %d\n", tape_flag);

		strcpy( dfilename, entry.filename);
	  }
	 if (t_read(tape,(char*) tape_buffer,CBUFFER_SIZE*tape_buffer_size)	
				< CBUFFER_SIZE) {	
	  printf("<I> End of file reached\n");
	  t_close(tape);
	  file_open = 0;
	  return 1;
	  }
	 tape_buffer_pointer=tape_buffer_size;
	 }
	tape_buffer_pointer--;
	memcpy((char*) buffer
		,(char*) tape_buffer + tape_buffer_pointer*CBUFFER_SIZE
		,CBUFFER_SIZE);
	break;
		
 case S_TAPELIST :
  if (!tape_buffer_pointer) {
    if (!tape) {
      printf("<?> Insert new tape <y/n> ? ");
      fflush(stdout);
      do {
	scanf("%c",&input);
      } while ( (input != 'y') && (input != 'n') );
      if (input != 'y') {
	printf("<?> So why did you start ?\n");
	return -1;
      } 
      if ((tape=t_mount(tapename))==NULL) {
	perror("<E> Cannot mount tape");
	fprintf(stderr,"    Tape = %s\n",tapename);
	return -1;
      }
      printf("    Tape %s mounted\n",tape->label);
      tape->status |= T_USE_HEADER;
      file_open = 0;
    }
    if (!file_open) {
      TAPE_ENTRY* help_entry;
      int	       use_file = 0;
      int	       ign_file = 0;
      static int       stopped  = 0;
      if (stopped==1) 	{
	help_entry = t_get_current_entry(tape);
	stopped = 2;
      }
      else {
	help_entry = t_get_next_entry(tape);
	stopped = 0;
      }
      while (!file_open) {
	if (help_entry->file_sequence_number < 0) {
	  printf("<I> End of tape reached.\n");
	  printf("    Unmounting ... ");
	  fflush(stdout);
	  t_umount(tape);
	  tape = NULL;
	  free(help_entry);
	  printf("Done.\n");
	  return -1;
	}			       /* check if file is to analyze */
	sscanf(help_entry->filename,"%s",help);
	use_file = 0;
	ign_file = 0;
	for (i=0;i<tape_filepointer;i++) {	         
	  if (ku_match(help,tape_filelist[i],1)) { 
	    switch(tape_fileaction[i]) {
	     case TF_ACCEPT:
	      use_file = 1; break;
	     case TF_IGNORE:
	      ign_file = 1; break;
	     case TF_STOP:
	      if (!stopped) {
		printf("<I> Stop request\n");
		fflush(stdout);
		stopped = 1;
		free(help_entry); 
		return -1;
	      }
	      break;
	     case TF_END:
	      printf("<I> EOT request\n");
	      printf("    Unmounting ... ");
	      fflush(stdout);
	      t_umount(tape);
	      tape = NULL;
	      free(help_entry);
	      printf("Done.\n");
	      return -1;
	      break;
	    }
	  } /* End ku_match */
	}			      
	if ( (use_file==1) && (ign_file==0)) {	/* use file ? */
	  entry = *help_entry;
	  free(help_entry);
	  if (t_open(tape,T_READ,NULL)<0) {
	    perror("<E> Error opening tape file, unmounting tape\n");
	    fprintf(stderr,"    File = %s\n",entry.filename);
	    t_umount(tape);
	    tape = NULL;
	    return -1;
	  }
	  printf("<I> New tape file opened: %s\n",entry.filename);
	  process_newfile(entry.filename);
	  file_has_changed = 1;
	  file_open = 1;

		if ( (s1 = rindex( entry.filename, '/')) != NULL) s2 = s1 + 5;	// This is for disc files
		else s2 = entry.filename + 4;								// This is for tape files
		s2[3] = '\0';
		run_char = s2[2];
		if ( run_char < 97) run_char += 32;					// This converts capital letters
		s2[2] = '\0';										// to small ones
		run_num = atoi( s2);
		printf( "    Run ID is: DPI %2d - %c\n", run_num, run_char);
		printf( "    File Counter = %3d\n", ++file_count);

		if ( run_num < 7) veto5_flag = 0;						// Runs before "dpi_07f" should use
		else if ( run_num > 7) veto5_flag = 1;					// the normal veto 5 position.  The
		else if ( run_char <= 101) veto5_flag = 0;				// rest should use the newer position.
		else if ( run_char > 101) veto5_flag = 1;				// See "unpack_event.c."

//		if ( veto5_flag == 0) printf( "    Using setno = 54 for Veto #5\n");
//		else if ( veto5_flag == 1) printf( "    Using setno = 63 for Veto #5\n");

		if ( run_num < 8) tape_flag = 1;
		else if ( run_num < 14) tape_flag = 2;
		else tape_flag = 3;
//		printf( "    Tape Number is %d\n", tape_flag);

		strcpy( dfilename, entry.filename);
	}
	else {		       /* get next tape entry */
	  free(help_entry);
	  help_entry = t_get_next_entry(tape);
	}
      } /* end: while file not open */
    } /* end: if file not open */
    
    if (t_read(tape,(char*) tape_buffer,CBUFFER_SIZE*tape_buffer_size)	
	< CBUFFER_SIZE) {	
      printf("<I> End of file reached\n");
      t_close(tape);
      file_open = 0;
      return 1;
    }
    tape_buffer_pointer=tape_buffer_size;
  }
  tape_buffer_pointer--;
  memcpy((char*) buffer
	 ,(char*) tape_buffer + tape_buffer_pointer*CBUFFER_SIZE
	 ,CBUFFER_SIZE);
  break;
#endif
  default :
	fprintf(stderr,"<E> Internal error: unknown source\n");
	return -1;
  }

if (byteorder) {
  unsigned char  temp;
  unsigned char* b;
  b = (unsigned char*) buffer;
  for (i=0;i<CBUFFER_SIZE;i+=2) {
	  temp   = b[i];
	  b[i]   = b[i+1];
	  b[i+1] = temp;
	  }
  } 

if (wordorder) {
  unsigned short  temp;
  unsigned short* b;
  b = (unsigned short*) buffer;
  for (i=0;i<SBUFFER_SIZE;i+=2) {
	  temp   = b[i];
	  b[i]   = b[i+1];
	  b[i+1] = temp;
	  }
  } 

  ana_def.buffer ++;
return 0;
}

/****************************************************************************/

void end_get_buffer()
{

  printf("<I> Stop getting buffer.\n");
  
  if (!source) return;  

  switch (source) {
   case S_TCPIP    :
	if (file_open) {
	  printf("    Closing TCPIP connection to %s (%d)\n",
				buffer_server, buffer_port);
	  send_int(buffer_socket, TCP_STOP);
	  close(buffer_socket);
	  file_open = 0;
	}
	break;
   case S_FILE     :
	if (file_open) {
	  printf("    Close data file %s\n",filename);
	  fclose(filepointer);
	  file_open = 0;
	  }
	break;
   case S_FILELIST :
	if (file_open) {
	  printf("    Close data file %s\n",filename);
	  fclose(filepointer);
	  file_open = 0;
	  }
	if (listpointer) {
	  printf("    Close list file %s\n",listname);
	  fclose(listpointer);
	  }
	break;
#if defined(OSF1) || defined(ULTRIX)
   case S_TAPE     :
	if (file_open) {
	  printf("    Close data file %s\n",entry.filename);
	  t_close(tape);
	  file_open = 0;
	  }
	if (tape) {
	  printf("    Unmount tape %s\n",tape->label);
	  t_umount(tape);
	  }
	break;
   case S_TAPELIST :
	if (file_open) {
	  printf("    Close data file %s\n",entry.filename);
	  t_close(tape);
	  file_open = 0;
	  }
	if (listpointer) {
	  printf("    Close list file %s\n",listname);
	  fclose(filepointer);
	  }
	if (tape) {
	  printf("    Unmount tape %s\n",tape->label);
	  t_umount(tape);
	  }
	break;
#endif
  default :
	fprintf(stderr,"<E> Internal error: unknown source\n");
  }
  
return;
}

/****************************************************************************/

int   extension_get_buffer(int socket, control_buffer* cb)
{
  char	key[40],arg1[40],arg2[40];
  char  temp[20],item[120];
  int   start,stop;
  int   count,i;
  
  sscanf(cb->buffer.c,"%s %s %s",key,arg1,arg2);

  if (!strcmp(key,"BUFFER_LIST")) {

    start = atoi(arg1);
    stop  = atoi(arg2);
    
    if ( (start<1)|| (start>4096)) start =    1;
    if ( (stop<1) || (stop>4096))  stop  = 1024;

    for(i=(start-1);i<stop;i+=8) {
      sprintf(item,"%-5d: %-4x %-4x %-4x %-4x %-4x %-4x %-4x %-4x",
	      i,
	      ((unsigned short*) current_buffer)[i],
	      ((unsigned short*) current_buffer)[i+1],
	      ((unsigned short*) current_buffer)[i+2],
	      ((unsigned short*) current_buffer)[i+3],
	      ((unsigned short*) current_buffer)[i+4],
	      ((unsigned short*) current_buffer)[i+5],
	      ((unsigned short*) current_buffer)[i+6],
	      ((unsigned short*) current_buffer)[i+7]);
      send_message(socket,LIST_ITEM,item);
    }
    
    send_answer(socket,LIST_TRAILER);
    return 1;
  }
  else if (!strcmp(key,"BUFFER_STATUS")) {
    sprintf(item,"Current source is %s",ana_def.a_source);
    send_message(socket,LIST_ITEM,item);
    if (file_open) 
      send_message(socket,LIST_ITEM,"Source is open to read");
    else
      send_message(socket,LIST_ITEM,"Source not opened");
    switch(source) {
     case S_TCPIP:
      sprintf(item,"Server: %s",buffer_server);
      send_message(socket,LIST_ITEM,item);
      sprintf(item,"Port  : %d",buffer_port);
      send_message(socket,LIST_ITEM,item);
      break;
#if defined(OSF1) || defined(ULTRIX)
     case S_TAPELIST:
      sprintf(item,"Current Listfile: %s",listname);
      send_message(socket,LIST_ITEM,item);
     case S_TAPE:
      sprintf(item,"Current Filename: %s",filename);
      send_message(socket,LIST_ITEM,item);
      sprintf(item,"Mounted Tape    : %s on %s",tape->label,tapename);
      send_message(socket,LIST_ITEM,item);
      break;
#endif
     case S_FILELIST:
      sprintf(item,"Current Listfile: %s",listname);
      send_message(socket,LIST_ITEM,item);
     case S_FILE:
      sprintf(item,"Current Filename: %s",filename);
      send_message(socket,LIST_ITEM,item);
      break;
    }
    
    send_answer(socket,LIST_TRAILER);
    return 1;
    
  } /* end buffer_status */
  
  else if (!strcmp(key,"HELP")) {
    send_message(socket,LIST_ITEM,"--------");
    send_message(socket,LIST_ITEM,"Section: GET_BUFFER");
    send_message(socket,LIST_ITEM,"  BUFFER_LIST [<startword> [<stopword>]]");
    send_message(socket,LIST_ITEM,"  BUFFER_STATUS");
    send_message(socket,LIST_ITEM,"--------");
    return 2;
  }

  return 0;
}

/****************************************************************************/
