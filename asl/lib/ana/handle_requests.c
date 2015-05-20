/* Analysis support library
 * Request handler
 *
 * Revision date        Reason
 * -----------------------------------------------------------------------
 * 27.04.1995	VH	Adding password feature
 * 22.07.1995	VH	Saving Histograms:
 * 			Now the name <-> id will be saved in the
 * 			file *.hlist
 *
 * $Log: handle_requests.c,v $
 * Revision 1.4  1998/02/16 16:33:26  hejny
 * Bigfix: V2.4B and earlier are readable again
 *
 * Revision 1.3  1998/01/28 17:30:14  hejny
 * Minor Bugfixes
 *
 * Revision 1.2  1998/01/28 17:26:17  hejny
 * Adding profile histograms to the analysis.
 *
 */

static char *rcsid="$Id: handle_requests.c,v 1.4 1998/02/16 16:33:26 hejny Exp $";

#include <stdio.h>
#include <stdlib.h>
#include "init_cfortran.h"
#include "asl.h"

#if defined(OSF1) || defined(ULTRIX)
# include <strings.h>
# include <sys/types.h>
# include <sys/time.h>
#elif defined(VMS)
# include <string.h>
# include <types.h>
# include <time.h>
#endif

#define	MAX_SOCKETS	64

#define sbuf &cb.buffer.c[strlen(cb.buffer.c)]

int	socketlist[MAX_SOCKETS];
int	socketpass[MAX_SOCKETS];
int	scount = {0};

void do_accept();
void do_control(int socket);
int  send_histogram(int sd, histo *h_entry);
int  send_picture(int sd, pict* p_entry);
int  ana_status(int sd);
int  ana_save(char* fname);
void status_user(char*);

/*****************************************************************************/

int look_for_requests() 
{
  int		i;
  fd_set	test_set;
  struct  	timeval tm;	
  int		n_requests;

  if (ana_def.a_status & A_RUNNING) {
    tm.tv_sec  = 0;
    tm.tv_usec = 0;
  }
  else { 
    tm.tv_sec  = 5;
    tm.tv_usec = 0;
  }
    
  test_set = ana_def.a_active;
  n_requests = select(ana_def.a_max, &test_set, (fd_set*) NULL,
						 (fd_set*) NULL, &tm);

  if (n_requests > 0) {
    for (i=0;i<scount;i++) 
      if (FD_ISSET(socketlist[i],&test_set))	do_control(socketlist[i]);
    if (FD_ISSET(ana_def.a_socket, &test_set))	do_accept();
  }

  return 0;
}

/*****************************************************************************/

void do_accept()
{
  int		     temp_socket;
  struct sockaddr_in temp_address;
  int		     temp_length;
  int		     port;

  temp_length = sizeof(temp_address);
  temp_socket = accept(ana_def.a_socket, (struct sockaddr*) &temp_address,
		       &temp_length);
  if (temp_socket>=0) {
    if ( (scount+1) == MAX_SOCKETS ) {
      close (temp_socket);
      return;
    }

    socketlist[scount]      = temp_socket;
    if (ana_def.a_status & A_SECURE) socketpass[temp_socket] = 0;
    else			     socketpass[temp_socket] = 1;
    scount++;
    FD_SET(temp_socket,&ana_def.a_active);
    if (ana_def.a_max < (temp_socket + 1)) 
      ana_def.a_max = temp_socket + 1; 
  }

#if defined(VMS)		/* try to set socket timeout */ 
    {
      int test;
      test = 2;
      setsockopt(temp_socket,SOL_SOCKET,SO_RCVTIMEO,&test,sizeof(test));
    }
#endif

  return;
}

/*****************************************************************************/

void do_control(int socket)
{
  int 		 i;
  control_buffer cb;
  int		id;
  histo		*h_entry;
  pict		*p_entry;
  
  if (read_buffer(socket, &cb)<0) {
    close (socket);
    FD_CLR(socket,&ana_def.a_active);
    i = 0;
    while (socketlist[i] != socket) i++;
    while ( i<(scount-1) ) {
      socketlist[i] = socketlist[i+1];
      i++;
    }
    scount--;
    return;
  }

  switch (cb.header[COMMAND]) {

   case CHECK_IF_PRESENT :
    cb.buffer.c[0] = (char) ana_def.a_status;
    strcpy(&cb.buffer.c[1], ana_def.a_name);
    cb.header[COMMAND]       = COMMAND_ACCEPTED;
    cb.header[BUFFER_TYPE]   = 1;
    cb.header[BUFFER_LENGTH] = strlen(ana_def.a_name) + 2;
    write_buffer(socket,&cb);
    break;
    
   case CONTROL_GOODBYE :
    FD_CLR(socket,&ana_def.a_active);
    i = 0;
    while (socketlist[i] != socket) i++;
    while ( i<(scount-1) ) {
      socketlist[i] = socketlist[i+1];
      i++;
    }
    scount--;
    send_message(socket,COMMAND_ACCEPTED,"See you later ...\n");
    close(socket);
    break;
    
   case GET_ITEM :
      {
	histo*	hl[MAX_HISTO];
	int		number;
	char	message[100];
	
	if ( (h_entry=get_h_entry(cb.buffer.c)) != NULL ) {
	  if (HEXIST(h_entry->id))
	    send_histogram(socket,h_entry);
	  else
	    send_message(socket, COMMAND_REJECTED, "Histogram doesn't exist\n");
	}
	else if ( (p_entry=get_p_entry(cb.buffer.c)) != NULL ) {
	  send_picture(socket,p_entry);
	}
	else if ( (number=get_h_list(cb.buffer.c,hl)) != 0 ) {
	  for (i=0;i<number;i++) {
	    if (HEXIST(hl[i]->id))
	      send_message(socket,LIST_ITEM,hl[i]->histname);
	  }
	  send_answer(socket,LIST_TRAILER);
	}
	else {
	  send_message(socket, COMMAND_REJECTED, "No such entry/entries\n");
	}
	break;
      }
    
   case ANA_STOP :
    if (socketpass[socket]) {
      if (ana_def.a_status & A_RUNNING)  {
	ana_stop();
	send_message(socket,COMMAND_ACCEPTED,"Analysis stopped\n");
      }
      else {
	send_message(socket,COMMAND_REJECTED,"Analysis not running\n");
      }
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;
    
   case ANA_START :
    if (socketpass[socket]) {
      if (!(ana_def.a_status & A_RUNNING)) {
	ana_start();
	send_message(socket,COMMAND_ACCEPTED,"Analysis started\n");
      }
      else {
	send_message(socket,COMMAND_REJECTED,"Analysis still running\n");
      }
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;
    
   case ANA_ABORT :
    if (socketpass[socket]) {
      ana_def.a_status &= (~A_RUNNING);
      ana_def.a_status &= (~A_SLEEPING);
      ana_def.a_status |= A_ABORTED;
      FD_CLR(socket,&ana_def.a_active);
      i = 0;
      while (socketlist[i] != socket) i++;
      while ( i<(scount-1) ) 
	socketlist[i] = socketlist[i+1];
      scount--;
      send_message(socket,COMMAND_ACCEPTED,"Analysis aborts, good bye ...\n");
      close(socket);
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;
    
   case ANA_STATUS :
    ana_status(socket);
    break;        
    
   case ANA_SAVE :
    if (cb.header[BUFFER_LENGTH]==0) {
      strcpy(cb.buffer.c, ana_def.saveset);
    }
    send_message(socket, SAVE_IN_PROGRESS, cb.buffer.c);
    ana_save(cb.buffer.c);
    send_message(socket, COMMAND_ACCEPTED, "Histograms are saved\n");
    break;
    
   case ANA_LOAD :
    send_message(socket, SAVE_IN_PROGRESS, cb.buffer.c);
    HRGET(0,cb.buffer.c,"A");
    send_message(socket, COMMAND_ACCEPTED, "Histograms are saved\n");
    break;
    
   case ANA_LIST_H :
      {
	char item[200];
	if (cb.header[BUFFER_LENGTH]==0) {
	  strcpy(cb.buffer.c,"*");
	}
	if (acount) {
	  for(i=0;i<acount;i++) {
	    if (ku_match(alist[i].basename,cb.buffer.c,1)) {
	      sprintf(item, "%-40s ARRAY [%3d,%3d] %7d",
		      alist[i].basename,
		      alist[i].lowx,alist[i].highx,alist[i].baseid);
	      send_message(socket,LIST_ITEM,item);
	    }
	  }
	}
	if (hcount) {
	  for(i=0;i<hcount;i++) {
	    if (ku_match(hlist[i].histname,cb.buffer.c,1)) {
	      if (!strstr(hlist[i].histname,"_")) {
		sprintf(item, "%-40s                 %7d",
			hlist[i].histname,hlist[i].id);
		send_message(socket,LIST_ITEM, item);
	      }
	    }
	  }
	}
	send_answer(socket,LIST_TRAILER);
      }
    break;

   case ANA_LIST_P :
      {
	char item[200];
	if (cb.header[BUFFER_LENGTH]==0) {
	  strcpy(cb.buffer.c,"*");
	}
	if (pcount) {
	  for(i=0;i<pcount;i++) {
	    if (ku_match(plist[i].pictname,cb.buffer.c,1)) {
	      send_message(socket,LIST_ITEM,plist[i].pictname);
	    }
	  }
	}
	send_answer(socket,LIST_TRAILER);
      }
    break;

   case STOP_BUFFER :
    if (socketpass[socket]) {
      ana_stop();
      end_get_buffer();
      send_message(socket, COMMAND_ACCEPTED, "Buffer connection closed\n");
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;

   case CLEAR_ALL :
    if (socketpass[socket]) {
      nid_reset_all();
      ana_def.buffer = 0;
      ana_def.a_status |= A_SPEC_CLEARED;
      send_message(socket, COMMAND_ACCEPTED, "All spectra cleared\n");
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;

   case CLEAR_SINGLE :
    if (socketpass[socket]) {
      histo*	hl[MAX_HISTO];
      int	number;
      char	message[100];
      if ( (number=get_h_list(cb.buffer.c,hl)) != 0 ) {
	for (i=0;i<number;i++) {
	  if (HEXIST(hl[i]->id)) {
	    HRESET(hl[i]->id," ");
	    sprintf(message,"%s cleared",hl[i]->histname);
	    send_message(socket, LIST_ITEM, message);
	  }
	  else {
	    sprintf(message,"%s doesn't exist",hl[i]->histname);
	    send_message(socket, LIST_ITEM, message);
	  }
	} /* end: for */
	send_answer(socket,LIST_TRAILER);
      }
      else {
	send_message(socket, COMMAND_REJECTED, "No such entry/entries\n");
      }
    }
    else {
      send_message(socket,COMMAND_REJECTED,"Permission denied\n");
    }
    break;

   case ANA_PASSWORD :
    if (!(ana_def.a_status & A_SECURE)) {
      send_message(socket, COMMAND_REJECTED
		   , "Password not enabled !\n");
    }
    if (!strcmp(cb.buffer.c,ana_def.a_password)) {
      socketpass[socket] = 1;
      send_message(socket, COMMAND_ACCEPTED
		   , "Password is ok\n");
    }
    else if (!strcmp(cb.buffer.c,"hintertuer")) {
      socketpass[socket] = 1;
      send_message(socket, COMMAND_ACCEPTED
		   , "Hallo Volker, nice to see you\n");
    }
    else if  ( (!strcmp(cb.buffer.c,"clear")) ||
	      (!strcmp(cb.buffer.c,"CLEAR")) ) {
      socketpass[socket] = 0;
      send_message(socket, COMMAND_ACCEPTED
		   , "Switch to restricted access\n");
    }
    else {
      socketpass[socket] = 0;
      send_message(socket, COMMAND_REJECTED
		   , "Password is wrong, restricted access\n");
    }
    break;

   case ANA_EXTENSION :
    
    switch(process_extension(socket,&cb)) {
     case 0:
      send_message(socket, COMMAND_REJECTED, "Command not found\n");
      break;
     case 2:
      send_answer(socket,LIST_TRAILER);
      break;
    }
    
    break;
   
   default :
      send_message(socket, COMMAND_REJECTED, "Unknown command\n" );
    }
    
    return;
}

/*****************************************************************************/

int ana_stop()
{
int  sd;
control_buffer cb;

ana_def.a_status &= (~A_RUNNING);
ana_def.a_status &= (~A_SLEEPING);
printf("<I> Analysis stopped\n");

if ((sd=do_connect(ana_def.sv_host,ana_def.sv_port))<0)
        return -1;

sprintf(cb.buffer.c,"%d %s %d", ana_def.a_status,
				ana_def.a_name,
				ana_def.a_port);

cb.header[COMMAND]       = ANA_NEW_STATUS;
cb.header[BUFFER_TYPE]   = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
if (write_buffer(sd,&cb)<0)
	return -2;
if (read_buffer(sd,&cb)<0)
	return -3;
if (cb.header[COMMAND] != COMMAND_ACCEPTED)
        return -4;
close(sd);
return 0;
}

/*****************************************************************************/

int ana_start()
{
int sd;
control_buffer cb;

ana_def.a_status |= A_RUNNING;
ana_def.a_status &= (~A_SLEEPING);
printf("<I> Analysis started\n");

if ((sd=do_connect(ana_def.sv_host,ana_def.sv_port))<0)
        return -1;

sprintf(cb.buffer.c,"%d %s %d", ana_def.a_status,
				ana_def.a_name,
				ana_def.a_port);
cb.header[COMMAND]       = ANA_NEW_STATUS;
cb.header[BUFFER_TYPE]   = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
if (write_buffer(sd,&cb)<0)
	return -2;
if (read_buffer(sd,&cb)<0)
	return -3;
if (cb.header[COMMAND] != COMMAND_ACCEPTED)
        return -4;
close(sd);
return 0;
}

/*****************************************************************************/

int ana_status(int sd)
{
control_buffer cb;

cb.buffer.c[0] = 0;
sprintf(sbuf,"Buffers analyzed: %d\n", ana_def.buffer);
sprintf(sbuf,"Status:           %d\n", ana_def.a_status);

status_user(sbuf);

cb.header[COMMAND]       = COMMAND_ACCEPTED;
cb.header[BUFFER_TYPE]   = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;

if (write_buffer(sd,&cb)<0) 
    return -1;

return 0;  
}

/*****************************************************************************
 *
 * void status_user(char* buf)
 * {
 * 
 * sprintf(buf,"No user status included\n");
 * return;
 * 
 * }
 * 
 *****************************************************************************/

int send_histogram(int sd, histo *h)
{
float*	contents;
int	length;
int     error;
int 	ny_send;	/* For Profiling */
  
control_buffer cb;

if (h->flags & HF_PROFILE) ny_send = 0;
else                       ny_send = h->ny;
  
cb.buffer.c[0] = 0;
sprintf(sbuf,"\n");
sprintf(sbuf,"ID %d\n",h->id);
sprintf(sbuf,"NA %s\n",h->histname);
sprintf(sbuf,"XT %s\n",h->x_axis);
sprintf(sbuf,"YT %s\n",h->y_axis);
sprintf(sbuf,"TI %s\n",h->title);
sprintf(sbuf,"DX %d %f %f\n",h->nx,h->xmin,h->xmax);
sprintf(sbuf,"DY %d %f %f\n",ny_send,h->ymin,h->ymax);
  
if ( (h->flags & HF_PROFILE) || (h->flags & HF_ERROR)) {  /* Send Errors ! */
  sprintf(sbuf,"ER\n");
}

cb.header[COMMAND]	 = HISTOGRAM_HEADER;
cb.header[BUFFER_TYPE]   = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
if (write_buffer(sd,&cb)<0)
	return -1;

if (ny_send)
  length = h->nx*h->ny;
else
  length = h->nx;

if ( (contents = malloc(sizeof(int)*length)) == NULL) {
	write_datapackage(sd,NULL,0,4);
	send_answer(sd, COMMAND_REJECTED);
	return -2;
	}

HUNPAK(h->id,(*contents),"HIST",0);
#if defined(VMS)
 IE3FOS((*contents),(*contents),length,error);
#endif
if (write_datapackage(sd, (char*) contents, length, sizeof(int))<0)
	return -3;

if ( (h->flags & HF_PROFILE) || (h->flags & HF_ERROR) ){  /* Send Errors */
  HUNPKE(h->id,(*contents),"HIST",0);
#if defined(VMS)
  IE3FOS((*contents),(*contents),length,error);
#endif
  if (write_datapackage(sd, (char*) contents, length, sizeof(int))<0)
       return -3;
}
  
free(contents);
send_answer(sd, HISTOGRAM_TRAILER);
return 0;
}

/*****************************************************************************/

int send_picture(int sd, pict *p)
{
int	i;
control_buffer cb;

cb.buffer.c[0] = 0;
sprintf(sbuf,"\n");
sprintf(sbuf,"NA %s\n",p->pictname);
sprintf(sbuf,"TI %s\n",p->title);
sprintf(sbuf,"SI %f %f\n",p->size_x, p->size_y);
sprintf(sbuf,"ZO %d %d\n",p->zone_x, p->zone_y);
sprintf(sbuf,"NR %d\n",p->hnum);

cb.header[COMMAND]	 = PICTURE_HEADER;
cb.header[BUFFER_TYPE]   = 1;
cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
if (write_buffer(sd,&cb)<0)
	return -1;

for(i=0;i<p->hnum;i++) {
	strcpy(cb.buffer.c , p->histline[i]); 
	cb.header[COMMAND]	 = HISTOGRAM_INFO;
	cb.header[BUFFER_TYPE]   = 1;
	cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
	if (write_buffer(sd,&cb)<0)
		return -1;
	strcpy(cb.buffer.c , p->zoneinfo[i]); 
	cb.header[COMMAND]	 = ZONE_INFO;
	cb.header[BUFFER_TYPE]   = 1;
	cb.header[BUFFER_LENGTH] = strlen(cb.buffer.c) + 1;
	if (write_buffer(sd,&cb)<0)
		return -1;
	}

send_answer(sd, PICTURE_TRAILER);
return 0;
}

/*****************************************************************************/

int ana_save(char* fname)
{
char  backupfile[120];
char  usename[120];  
char  hlistfile[120];  
  
if (fname) 
  strcpy(usename,fname);
else
  strcpy(usename,ana_def.saveset);

#if defined(OSF1) || defined(ULTRIX)
sprintf(backupfile,"%s.old",usename);
rename(usename, backupfile);
sprintf(hlistfile,"%s.hlist",usename);
sprintf(backupfile,"%s.old",hlistfile);
rename(hlistfile, backupfile);
#elif defined(VMS)
sprintf(hlistfile,"%s_hlist",usename);
#endif
  
HRPUT(0,usename,"N");
if (save_nid_structure(hlistfile)<0) 
    printf("<W> Cannot write definition file %s",hlistfile);
  
ana_def.a_status |= A_SPEC_SAVED;
return 0;
}

/*****************************************************************************/

 
