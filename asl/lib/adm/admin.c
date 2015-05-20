#define NO_CFORTRAN

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "asl.h"

#if defined(OSF1) || defined(ULTRIX)
# include <unistd.h>
# include <strings.h>
# include <pwd.h>
#elif defined(VMS)
# include <string.h>
# include <file.h>
#endif

int	DEBUG = {0};

char	statusfile[100];
char	logfile[100];
int	lfp;

char	log[100];

analist	al[MAX_ANA];

#if defined(OSF1) || defined(ULTRIX)
struct passwd *pwd;
#endif
char adm_owner[20];

void process_request(int socket, control_buffer* cb, struct sockaddr_in* fa);
void print_adm_help();
int  save_analist(char* file, analist* list);
int  read_analist(char* file, analist* list);
void check_all_analysis();


/******************************************************************************/

main(int argc, char** argv)
{
int 	i;
int	current_time;
int	test_socket;
int	listen_socket;
int	request_socket;

fd_set	listen_fs;

int	length;

struct sockaddr_in foreign_address;

control_buffer cb;

				/* default values */
strcpy(logfile,LOGFILE);
strcpy(statusfile,STATUSFILE);

#if defined(OSF1) || defined(ULTRIX)
pwd = getpwuid(getuid());
strcpy(adm_owner,pwd->pw_name);
#else
adm_owner[0]=0;
#endif
				/* first look at command line */
if (argc>1) {
  for(i=1;i<argc;i++) {
    if 	(!strcmp("-h",argv[i]))		
      print_adm_help(); 
    else if (!strcmp("-l",argv[i])) 
      strcpy(logfile,argv[i+1]);
    else if (!strcmp("-s",argv[i]))
      strcpy(statusfile,argv[i+1]);
    else if (!strcmp("-d",argv[i]))
      DEBUG = 1;	
  }
}

				/* open logfile */
if ((lfp=open(logfile, O_WRONLY | O_CREAT | O_APPEND, 0644)) <0) {
	perror("<E> Cannot open logfile");
	exit(1);
	}

current_time = time(NULL);
sprintf(log,"*** ADMINISTRATOR started at %s",ctime( (time_t*) &current_time));
write(lfp,log,strlen(log));

if (DEBUG)
	fprintf(stderr,"<D> Logfile opened: %s\n",logfile);

				/* administrator still running ? */
test_socket = do_connect("localhost",SERVER_PORT);
if (test_socket >= 0) {		/* it is running */
	fprintf(stderr,"Administrator is still running\n");
	if (send_single_command(test_socket, CHECK_IF_PRESENT)<0)
		fprintf(stderr,"... but transmission problems\n");
	exit(1);
	}
else {
	sprintf(log,"<I> Initial run test result: %d\n",test_socket);
	write(lfp,log,strlen(log));
	}

if (DEBUG)
	fprintf(stderr,"<D> Initial check result: %d\n",test_socket);

				/* check status file and connect */
				/* listed analyses */
if (read_analist(statusfile, al)<0) {
	sprintf(log,"<W> Cannot read statusfile\n");
	write(lfp,log,strlen(log));
	if (DEBUG)
		fprintf(stderr,"<D> Cannot read statusfile\n");
				/* init analist */
	for(i=0;i<MAX_ANA;i++)
		al[i].status = 0;	
	}
else {
	check_all_analysis();
	} /* end else */
		
				/* set listen mode */	
listen_socket = install_listen(SERVER_PORT);
if (listen_socket<0) {
	perror("Error installing listen");
	exit(1);
	}


while(1) {
  FD_ZERO(&listen_fs);
  FD_SET(listen_socket,&listen_fs);

  if (DEBUG)
	fprintf(stderr,"<D> Waiting for select ...\n");

  select(listen_socket+1, &listen_fs, (fd_set*) 0, (fd_set*) 0, 0);

  if (FD_ISSET(listen_socket, &listen_fs)) {
     length = sizeof(foreign_address);
     request_socket = accept(listen_socket,
				 (struct sockaddr*) &foreign_address, &length);
     if (request_socket >= 0) {
	char	peername[100];
	int	peerport;
	int 	status;

	strcpy(peername, addr2host(&foreign_address, &peerport));   
	sprintf(log,"<I> Request: %s port %d\n",peername, peerport);
	write(lfp,log,strlen(log));

	if ((status=read_buffer(request_socket, &cb))<0) {
	  sprintf(log," *  ERROR reading buffer from host: %d\n",status);
	  write(lfp,log,strlen(log));
	  if (DEBUG)
	     fprintf(stderr,"<D> ERROR reading buffer from host: %d\n",status);
	  }
	else {
	  process_request(request_socket,&cb,&foreign_address);	
	  }

	close(request_socket);
	} /* end if request socket >= 0 */
     else {
	sprintf(log," *  ERROR getting socket from accept: %d\n",
							request_socket);
	write(lfp,log,strlen(log));
 	if (DEBUG)
	   fprintf(stderr,"<D> Error getting socket from accept: %d\n",
							request_socket);
	}
     } /* end if socket is set */
  else {
     sprintf(log," *  WARNING, select sees unset socket\n");
     write(lfp,log,strlen(log));
     if (DEBUG)
	   fprintf(stderr,"<D> Select sees unset socket\n");
     }

  } /* end while */

}

/******************************************************************************/
				
void process_request(int socket, control_buffer* cb, struct sockaddr_in* fa)
{
int i;
int port;

char	a_name[100];
char	a_owner[100];
char	a_host[100];
int	a_port,a_status;
int	argnum;
  
if (DEBUG)
	fprintf(stderr,"<D> Request to process: %d\n",cb->header[COMMAND]);

switch(cb->header[COMMAND]) {

  case CHECK_IF_PRESENT :
	sprintf(log," *  Command arrived CHECK_IF_PRESENT\n");
	write(lfp,log,strlen(log));
	cb->header[COMMAND] = COMMAND_ACCEPTED;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	break;

  case ANA_HERE_I_AM :
	sprintf(log," *  Command arrived ANA_HERE_I_AM\n");
	write(lfp,log,strlen(log));
	i=0;
	while ( (al[i].status) && (i<MAX_ANA) )
	  i++;
	if (i==MAX_ANA) {
	  cb->header[COMMAND] = COMMAND_REJECTED;
	  sprintf(log," *  Maximum number of entries reached\n");
	  write(lfp,log,strlen(log));
	  }
	else {
	  argnum = sscanf(cb->buffer.c,"%d %s %s",&a_status ,a_name, a_owner);
	  if (argnum<3) a_owner[0]=0;
	  al[i].status = a_status;
	  if (a_status & A_STABLE) al[i].no_check = 0;
	  else		     	   al[i].no_check = 1;
	  strcpy(al[i].name,a_name);
	  strcpy(al[i].host,addr2host(fa,&al[i].port));	  
          strcpy(al[i].owner,a_owner);
	  cb->header[COMMAND] = COMMAND_ACCEPTED;
	  sprintf(log," *  NEW ANALYSES started: %s\n",al[i].name);
	  write(lfp,log,strlen(log));
	  sprintf(log," *      %s(%d)[%s] STATUS %d\n",al[i].host
		  				      ,al[i].port
		  				      ,al[i].owner
						      ,al[i].status);
	  write(lfp,log,strlen(log));
	  }
	cb->header[BUFFER_LENGTH] = 0;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	if (save_analist(statusfile, al)<0) {
	  sprintf(log,"<W> Cannot write statusfile\n");
	  write(lfp,log,strlen(log));
	  }
	break;
  case ANA_NEW_STATUS :
	sprintf(log," *  Command arrived ANA_NEW_STATUS\n");
	write(lfp,log,strlen(log));
	sscanf(cb->buffer.c,"%d %s %d",&a_status ,a_name, &a_port);
	strcpy(a_host,addr2host(fa,&port));
	i=0;
	while (( strcmp(al[i].name,a_name) ||
		 strcmp(al[i].host,a_host) ||
		(al[i].port != a_port)) && (i<MAX_ANA))
	  i++;
	if (i==MAX_ANA) {
	  cb->header[COMMAND] = COMMAND_REJECTED;
	  sprintf(log," *  Entry not found: %s\n",a_name);
	  write(lfp,log,strlen(log));
	  }
	else {
	  cb->header[COMMAND] = COMMAND_ACCEPTED;
	  al[i].status = a_status;
	  }
	cb->header[BUFFER_LENGTH] = 0;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	break;
  case ANA_GOODBYE :
	sprintf(log," *  Command arrived ANA_GOODBYE\n");
	write(lfp,log,strlen(log));
	sscanf(cb->buffer.c,"%d %s %d",&a_status ,a_name, &a_port);
	strcpy(a_host,addr2host(fa,&port));
	i=0;
	while (( strcmp(al[i].name,a_name) ||
		 strcmp(al[i].host,a_host) ||
		(al[i].port != a_port)) && (i<MAX_ANA))
	  i++;
	if (i==MAX_ANA) {
	  cb->header[COMMAND] = COMMAND_REJECTED;
	  sprintf(log," *  Entry not found: %s\n",a_name);
	  write(lfp,log,strlen(log));
	  sprintf(log," *      %s(%d) STATUS %d\n",a_host,a_port,a_status);
	  write(lfp,log,strlen(log));
	  }
	else {
	  cb->header[COMMAND] = COMMAND_ACCEPTED;
	  al[i].status = 0;
	  sprintf(log," *  Entry deleted: %s\n",a_name);
	  write(lfp,log,strlen(log));
	  }
	cb->header[BUFFER_LENGTH] = 0;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	if (save_analist(statusfile, al)<0) {
	  sprintf(log,"<W> Cannot write statusfile\n");
	  write(lfp,log,strlen(log));
	  }
	break;
  case ANA_FORCED_DELETE :
	{
	char	a_name[100];
	char	a_host[100];
        char	a_owner[100];
        int	a_port;

	sprintf(log," *  Command arrived ANA_FORCED_DELETE\n");
	write(lfp,log,strlen(log));

	sscanf(cb->buffer.c,"%d %s %d %s %s",&a_status ,a_name, &a_port
	       				    , a_host, a_owner);

	i=0;
	while (( strcmp(al[i].name,a_name) ||
		 strcmp(al[i].host,a_host) ||
		(al[i].port != a_port)) && (i<MAX_ANA))
	  i++;

	if (i==MAX_ANA) {
	  cb->header[COMMAND] = COMMAND_REJECTED;
	  sprintf(log," *  Entry not found: %s@%s(%d)\n",a_name,a_host,a_port);
	  write(lfp,log,strlen(log));
	}
	else {
	  if (  strcmp(al[i].owner,a_owner) &&
	      (strlen(al[i].owner)!=0) && (strlen(adm_owner)!=0) &&
	      strcmp(a_owner,adm_owner) ) {
	    cb->header[COMMAND] = COMMAND_REJECTED;
	    sprintf(log," *  No Permission: %s@%s(%d)\n",a_name,a_host,a_port);
	    write(lfp,log,strlen(log));
	  }
	  else {  
	    cb->header[COMMAND] = COMMAND_ACCEPTED;
	    al[i].status = 0;
	    sprintf(log," *  Entry deleted: %s@%s(%d)\n",a_name,a_host,a_port);
	    write(lfp,log,strlen(log));
	  }
	}
	cb->header[BUFFER_LENGTH] = 0;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	if (save_analist(statusfile, al)<0) {
	  sprintf(log,"<W> Cannot write statusfile\n");
	  write(lfp,log,strlen(log));
	  }
	}
	break;
  case CTR_ANALIST :
	{
	check_all_analysis(); 
	sprintf(log," *  Command arrived CTR_ANALIST\n");
	write(lfp,log,strlen(log));
	cb->header[COMMAND]     = COMMAND_ACCEPTED;
	cb->header[BUFFER_MODE] = WAIT_FOR_MORE;
	cb->header[BUFFER_TYPE] = 1;
	
	i=0;
	while(i<MAX_ANA) {
	  if (al[i].status) {
	    sprintf(cb->buffer.c,"%s %s %d %d %s",
				al[i].name,al[i].host,al[i].port
		               ,al[i].status,al[i].owner);
	    cb->header[BUFFER_LENGTH] = strlen(cb->buffer.c) + 1;
	    if (write_buffer(socket, cb)<0) {
	      sprintf(log," *  ERROR sending analyses data\n");
	      write(lfp,log,strlen(log));
	      }
	    } /* end status */
	  i++;
	  } /* end while */

	cb->header[BUFFER_MODE]   = DATA_COMPLETE;
	cb->header[BUFFER_LENGTH] = 0;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }
	}
	break;
  default :
	sprintf(log," *  WARNING, unknown request arrived\n");
	write(lfp,log,strlen(log));
	cb->header[COMMAND] = COMMAND_REJECTED;
	if (write_buffer(socket, cb)<0) {
	  sprintf(log," *  ERROR answering request\n");
	  write(lfp,log,strlen(log));
	  }

  } /* end switch */

}

/******************************************************************************/

void print_adm_help()
{

fprintf(stderr,"\n");
fprintf(stderr,"ASL Administrator\n");
fprintf(stderr,"Library: %s\n",ASL_VERSION);
fprintf(stderr,"\n");
fprintf(stderr,"-h        help\n");
fprintf(stderr,"-l file   logfile\n");
fprintf(stderr,"-s file   statusfile\n");
fprintf(stderr,"-d        debug\n");
fprintf(stderr,"\n");

exit(0);
}

/******************************************************************************/

int save_analist(char* file,analist* list)
{
int sfp;
int i;
char version[6];

if ((sfp=open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644))<0)
	return -1;

strcpy(version,"V2.6C");  
write(sfp, (char*) version, 5);
  
for(i=0;i<MAX_ANA;i++) {
  if (write(sfp,(char*) &list[i],sizeof(analist))<sizeof(analist)) {
	close(sfp);
	return -2;
	}
  }

close(sfp);
return 0;
} 

/******************************************************************************/

int read_analist(char* file,analist* list)
{
  int sfp;
  int i;
  char version[6];
  
  if ((sfp=open(file, O_RDONLY , 0644))<0)
    return -1;

  read(sfp, (char*) version, 5); 
  version[5] = 0;
  
  if (!strcmp(version,"V2.6C")) {
    for(i=0;i<MAX_ANA;i++) {
      if (read(sfp,(char*) &list[i],sizeof(analist))<sizeof(analist)) {
	close(sfp);
	return -2;
      }
    }
  }
  else {			       /* Old Version <2.6C */
    struct {
      char    name[20];
      char    host[100];
      int     port;
      int     status;
      int     no_check;
    } temp;
    
    close(sfp);
    if ((sfp=open(file, O_RDONLY , 0644))<0)
      return -1;
    for(i=0;i<MAX_ANA;i++) {
      if (read(sfp,(char*) &temp,sizeof(temp))<sizeof(temp)) {
	close(sfp);
	return -2;
      }
      strcpy(list[i].name,temp.name);
      strcpy(list[i].host,temp.host);
      list[i].port     = temp.port;
      list[i].status   = temp.status;
      list[i].no_check = temp.no_check;
      list[i].owner[0] = 0;
    }
  }
  
  close(sfp);
  return 0;
} 

/******************************************************************************/

void check_all_analysis() 
{
int 	i, test_socket;
control_buffer cb;

for (i=0;i<MAX_ANA;i++) {
  if ( al[i].status && (!al[i].no_check) ) {
    sprintf(log,"<I> Checking %s on host %s(%d)\n",
				al[i].name,al[i].host,al[i].port);
    write(lfp,log,strlen(log));
    test_socket = do_connect(al[i].host,al[i].port);
    if (test_socket<0) {
	al[i].status = 0;
	sprintf(log,"    ... not responding\n");	    
	write(lfp,log,strlen(log));
	}
    else {
	cb = send_command(test_socket, CHECK_IF_PRESENT);
	if (cb.header[COMMAND] == COMMAND_ACCEPTED) {
	   if (strcmp(&cb.buffer.c[1],al[i].name)) {
		strcpy(al[i].name,&cb.buffer.c[1]);
		sprintf(log,"    ... analyses changed: %s\n",al[i].name);
		write(lfp,log,strlen(log));
		}
	   al[i].status = (int) cb.buffer.c[0];
	   switch (al[i].status) {
	     case A_RUNNING :
		sprintf(log,"    ... analyses running\n");
		write(lfp,log,strlen(log));
	        break;
	     case A_SLEEPING :
		sprintf(log,"    ... analyses sleeping\n");
		write(lfp,log,strlen(log));
	        break;
	     default :
		sprintf(log,"    ... unknown status\n");
		write(lfp,log,strlen(log));
	        break;
	     } /* end switch */
	   } /* end command accepted */
	} /* end else */
    } /* edt if status */
  } /* end for */

return;
}

/*
int hexist_(int a) {}
int hi_(int a,int b) {}
int hx_(int a,int b) {}
int hrndm1_(int a) {}
*/
