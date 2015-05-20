/*
 *
 *	Library to use with TCP/IP Communication
 *
 *
 *	Contents:
 *
 *	macro set_tcp_timeout(int seconds)
 *
 *	int  netwrite(int socket, char* buffer, int length, int size)
 *	int  netread(int socket, char* buffer, int length, int size)
 *	void htons_a(short* buffer, int length)
 *	void ntohs_a(short* buffer, int length)
 *	void htonl_a(int* buffer, int length)
 *	void ntohl_a(int* buffer, int length)
 *
 *	int  install_listen(int port)
 *	int  do_connect(char* host, int port)
 *	struct sockaddr_in  host2addr(char* host, int port)
 *	char* 		    addr2host(struct sockaddr_in* address, int* port)
 *
 *	int  write_buffer(int socket,control_buffer* cb);
 *	int  read_buffer(int socket,control_buffer* cb);
 *	int  write_datapackage(int socket, char* buffer, int length, int size);
 *	int  read_datapackage(int socket, char* buffer, int max_length);
 *
 *	int  send_single_command(int socket, int command);
 *	int  send_message(int socket, int command, char* message);
 *	control_buffer send_command(int socket, int command);
 *
 *	int  send_int(int socket, int value);
 *	int  get_int(int socket);
 *
 *	int  wait_for_socket_r(int socket, int seconds); 
 *	int  wait_for_socket_w(int socket, int seconds); 
 */

#define NO_CFORTRAN

#include <stdlib.h>
#include <stdio.h>
#if defined(OSF1) || defined(ULTRIX)
# include <strings.h>
# include <sys/time.h>
# include <sys/types.h>
#elif defined(VMS)
# include <string.h>
# include <time.h>
# include <types.h>
# include <errno.h>
#endif
#include "asl.h"

int tcp_timeout = 15;
int tcp_errno   = 0;

/******************************************************************************/

int netwrite(int socket, char* buffer, int length, int size)
{
int temp;
int still_written;
int clength;

#if defined(VMS)
  errno=0;
#endif
  
tcp_errno = 0;
clength = size*length;

if (!clength) 
  return 0;

switch(size) {
  case 2 : htons_a( (short*) buffer, length );
	   break;
  case 4 : htonl_a( (int*) buffer, length );
	   break;
  }

still_written = 0;
do {
 if (wait_for_socket_w(socket,tcp_timeout)>0) {
#if defined(VMS)
   if (errno)		{
     			tcp_errno = TCP_INTERNAL;
     			return TCP_ERROR;
   			}
#endif
   temp = write(socket,&buffer[still_written],clength-still_written);
   if 	 (temp<=0)	{
     			tcp_errno = TCP_INTERNAL;
     			return TCP_ERROR;
   			}
   else			still_written += temp;
   }
 else {
   tcp_errno =  TCP_TIMEOUT;
   return TCP_ERROR;
   }
 } while (still_written<clength);

switch(size) {
  case 2 : ntohs_a( (short*) buffer, length );
	   break;
  case 4 : ntohl_a( (int*) buffer, length );
	   break;
  }

return TCP_OK;
}

/******************************************************************************/

int netread(int socket, char* buffer, int length, int size)
{
int temp;
int still_read;
int clength;

#if defined(VMS)
  errno=0;
#endif
  
tcp_errno = 0;
clength = size * length;

if (!clength)
 return 0;

still_read = 0;
do {
 if (wait_for_socket_r(socket,tcp_timeout)>0) {
#if defined(VMS)
   if (errno)		{
     			tcp_errno = TCP_INTERNAL;
     			return TCP_ERROR;
   			}
#endif
   temp = read(socket,&buffer[still_read],clength-still_read);
   if 	 (temp<=0)	{
          		tcp_errno = TCP_INTERNAL;
			return TCP_ERROR;
   			}
   else			still_read += temp;
   }
 else {
   tcp_errno =  TCP_TIMEOUT;
   return TCP_ERROR;
   }
 } while (still_read<clength);

switch(size) {
  case 2 : ntohs_a( (short*) buffer, length );
	   break;
  case 4 : ntohl_a( (int*) buffer, length );
	   break;
  }

return TCP_OK;
}

/******************************************************************************/

void htons_a(short* buffer, int length)
{
do {
   length--;
   buffer[length] = htons(buffer[length]);
   } while (length);

return;
}

/******************************************************************************/

void ntohs_a(short* buffer, int length)
{
do {
   length--;
   buffer[length] = ntohs(buffer[length]);
   } while (length);

return;
}

/******************************************************************************/

void htonl_a(int* buffer, int length)
{
do {
   length--;
   buffer[length] = htonl(buffer[length]);
   } while (length);

return;
}

/******************************************************************************/

void ntohl_a(int* buffer, int length)
{
do {
   length--;
   buffer[length] = ntohl(buffer[length]);
   } while (length);

return;
}

/******************************************************************************/

int install_listen(int port)
{
struct sockaddr_in	local_address;
int			listen_socket;
int			optval;

tcp_errno = 0;
                                        /* clear/fill address structure */
memset(&local_address,0 , sizeof(local_address) );
local_address.sin_family        = AF_INET;
local_address.sin_addr.s_addr   = htonl(INADDR_ANY);
local_address.sin_port          = htons(port);

                                        /* get socket */
listen_socket = socket(AF_INET,SOCK_STREAM,0);
if (listen_socket==-1) 	{
	tcp_errno =  TCP_INTERNAL;
	return TCP_ERROR;
	}

                                        /* set option reuseaddress */
optval = 1;
if (setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR, (char*) &optval
                                                        , sizeof(optval))<0) {
    close(listen_socket);
    tcp_errno = TCP_INTERNAL;
    return TCP_ERROR;
    }
                                        /* bind to local address */
if (bind(listen_socket, (struct sockaddr*) &local_address,
                                        sizeof(local_address))<0 ) {
        close(listen_socket);
	tcp_errno =  TCP_INTERNAL;
	return TCP_ERROR;
	}

                                        /* switch listen mode on */
if (listen(listen_socket,5)<0) {
        close(listen_socket);
	tcp_errno =  TCP_INTERNAL;
	return TCP_ERROR;
	}

return listen_socket;
}
 
/******************************************************************************/

int do_connect(char* hostname, int port)
{
struct sockaddr_in temp_address;
int temp_socket;
int i,sum;
int optval;

tcp_errno = 0;
temp_address = host2addr(hostname,port);

sum = 0;
for (i=0;i<sizeof(temp_address);sum += ((char*) &temp_address)[i++]);
if (sum==0) {
	tcp_errno =  TCP_UNKNOWN_HOST;
	return TCP_ERROR;
	}

if ( (temp_socket=socket(AF_INET,SOCK_STREAM,0)) < 0) {
	tcp_errno =  TCP_INTERNAL;
	return TCP_ERROR;
	}

                                        /* set option reuseaddress */
optval = 1;
if (setsockopt(temp_socket,SOL_SOCKET,SO_REUSEADDR, (char*) &optval
                                                        , sizeof(optval))<0) {
    close(temp_socket);
    tcp_errno = TCP_INTERNAL;
    return TCP_ERROR;
    }
    
if ( connect(temp_socket, (struct sockaddr*) &temp_address,
                                sizeof(temp_address)) < 0 ) {
	tcp_errno =  TCP_INTERNAL;
	return TCP_ERROR;
	}

return temp_socket;
}

/******************************************************************************/

struct sockaddr_in host2addr(char* hostname, int port)
{
static	struct sockaddr_in	sa;
struct hostent*			host;

tcp_errno = 0;
				/* clear address */
memset( &sa,0 , sizeof(sa) );

				/* get address */
if ( (host=gethostbyname(hostname))==NULL ) {
	tcp_errno =  TCP_UNKNOWN_HOST;
	return sa;
	}
				/* fill address */
sa.sin_family		= AF_INET;
sa.sin_port		= htons(port);
sa.sin_addr.s_addr	= *( (u_long*) (host->h_addr) );

return sa;
}

/******************************************************************************/

char* addr2host(struct sockaddr_in* address, int* port)
{
struct hostent*			host;

tcp_errno = 0;
					/* get hostname */
host = gethostbyaddr( (char*) &(address->sin_addr) ,
					sizeof(address->sin_addr), AF_INET);
if (host == NULL) {
	tcp_errno =  TCP_UNKNOWN_HOST; 
	return NULL;
	}

*port = ntohs(address->sin_port);
return host->h_name;
}

/******************************************************************************/

int write_buffer(int socket, control_buffer* cb)
{
int length;
int size;

tcp_errno = 0;
if (netwrite(socket, (char*) cb->header, HEADER_SIZE, 4)<0)	
	return TCP_ERROR;

size   = cb->header[BUFFER_TYPE];
length = cb->header[BUFFER_LENGTH];
if (netwrite(socket, cb->buffer.c, length, size)<0)	
	return TCP_ERROR;

return TCP_OK;
}

/******************************************************************************/

int read_buffer(int socket, control_buffer* cb)
{
int length;
int size;

tcp_errno = 0;
if (netread(socket, (char*) cb->header, HEADER_SIZE, 4)<0)	
	return TCP_ERROR;

size   = cb->header[BUFFER_TYPE];
length = cb->header[BUFFER_LENGTH];
if (netread(socket, cb->buffer.c, length, size)<0)
	return TCP_ERROR;

return TCP_OK;
}

/******************************************************************************/

int write_datapackage(int socket, char* buffer, int length, int size)
{
int 		clength;
control_buffer 	cb;
int		transmit_length; 
int		transmit_clength;
int		still_written;

tcp_errno = 0;
clength = length * size;
transmit_length  = CBUFFER_SIZE/size;
transmit_clength = transmit_length * size;

cb.header[COMMAND]	 = DATAPACKAGE;
cb.header[BUFFER_MODE]   = WAIT_FOR_MORE;
cb.header[BUFFER_TYPE]   = size;
cb.header[BUFFER_LENGTH] = transmit_length;

still_written = 0;

while (clength>CBUFFER_SIZE) {
	memcpy(cb.buffer.c, &buffer[still_written], transmit_clength);
	clength -= transmit_clength;
	still_written += transmit_clength;
	if (write_buffer(socket, &cb)<0)
		return TCP_ERROR;
	}

if (clength) {
	memcpy(cb.buffer.c, &buffer[still_written], clength);
	cb.header[BUFFER_MODE]   = DATA_COMPLETE;
	cb.header[BUFFER_LENGTH] = clength/size;
	if (write_buffer(socket, &cb)<0)
		return TCP_ERROR;
	}

return TCP_OK;
}


/******************************************************************************/

int read_datapackage(int socket, char* buffer, int max_length)
{
int 		clength;
control_buffer 	cb;
int		still_read;

tcp_errno = 0;
if (read_buffer(socket,&cb)<0)
	return TCP_ERROR;

if (cb.header[COMMAND] != DATAPACKAGE) {
	tcp_errno =  TCP_NODTP;
	return TCP_ERROR;
	}

still_read = 0;
while (cb.header[BUFFER_MODE]==WAIT_FOR_MORE) {
	clength = cb.header[BUFFER_TYPE] * cb.header[BUFFER_LENGTH];
	memcpy(&buffer[still_read], cb.buffer.c, clength);
	still_read += clength;
	if (read_buffer(socket, &cb)<0)
		return TCP_ERROR;
	}

clength = cb.header[BUFFER_TYPE] * cb.header[BUFFER_LENGTH];
memcpy(&buffer[still_read], cb.buffer.c, clength);
still_read += clength;

return still_read;
}

/******************************************************************************/

int send_single_command(int socket, int command)
{
control_buffer	cb;

tcp_errno = 0;
cb.header[COMMAND]		= command;
cb.header[BUFFER_LENGTH]	= 0;
cb.header[BUFFER_TYPE]		= 0;

if (write_buffer(socket,&cb)<0)
	return TCP_ERROR;

if (read_buffer(socket,&cb)<0)
	return TCP_ERROR;

/*
write_buffer(socket,&cb);
read_buffer(socket,&cb);
*/

if (cb.header[BUFFER_LENGTH]) printf("<A> %s",cb.buffer.c);

if (cb.header[COMMAND] != COMMAND_ACCEPTED) {
	tcp_errno =  TCP_NOACP;
	return TCP_ERROR;
	}

return TCP_OK;
}


/******************************************************************************/

int send_message(int socket, int command, char* message)
{
control_buffer	cb;

tcp_errno = 0;
cb.header[COMMAND]		= command;

if (message != NULL) {
	cb.header[BUFFER_LENGTH]	= strlen(message) + 1;
	strcpy(cb.buffer.c,message);
	cb.header[BUFFER_TYPE]		= 1;
	}
else {
	cb.header[BUFFER_LENGTH]	= 0;
	cb.header[BUFFER_TYPE]		= 0;
	}

if (write_buffer(socket,&cb)<0)
	return TCP_ERROR;

return TCP_OK;
}


/******************************************************************************/

control_buffer send_command(int socket, int command)
{
control_buffer	cb;

tcp_errno = 0;
cb.header[COMMAND]		= command;
cb.header[BUFFER_LENGTH]	= 0;
cb.header[BUFFER_TYPE]		= 0;


if (write_buffer(socket,&cb)<0) {
	cb.header[COMMAND] = -1;
	return cb;
	}

if (read_buffer(socket,&cb)<0) {
	cb.header[COMMAND] = -2;
	return cb;
	}

return cb;
}

/******************************************************************************/

int  send_int(int socket, int value)
{

tcp_errno = 0;
value=htonl(value);
if (wait_for_socket_w(socket,tcp_timeout)>0) {
  if ( -1 == send(socket,(char*)&value,4,0) ) {
     tcp_errno =  TCP_INTERNAL;
     return TCP_ERROR;
     }
  }
else {
     tcp_errno =  TCP_TIMEOUT;
     return TCP_ERROR;
  }

return TCP_OK;

}

/******************************************************************************/

int  get_int(int socket)
{
int value;

tcp_errno = 0;
if (wait_for_socket_r(socket,tcp_timeout)>0) {
  if ( -1 == read(socket,(char*) &value,4) ) {
     tcp_errno =  TCP_INTERNAL;
     return TCP_ERROR;
     }
  }
else {
     tcp_errno =  TCP_TIMEOUT;
     return TCP_ERROR;
  }

value=ntohl(value);

return value;

}

/******************************************************************************/

int  wait_for_socket_r(int socket, int seconds)
{
fd_set help;
struct timeval tm;

tcp_errno = 0;
tm.tv_sec  = seconds;
tm.tv_usec = 0; 

FD_ZERO(&help);
FD_SET(socket,&help);

if (seconds>0) 
	return select(socket+1,&help,(fd_set*) 0, (fd_set*) 0, &tm);
else
	return select(socket+1,&help,(fd_set*) 0, (fd_set*) 0, NULL);
}

/******************************************************************************/

int  wait_for_socket_w(int socket, int seconds)
{
fd_set help;
struct timeval tm;

tcp_errno = 0;
tm.tv_sec  = seconds;
tm.tv_usec = 0; 

FD_ZERO(&help);
FD_SET(socket,&help);

if (seconds>0) 
	return select(socket+1,(fd_set*) 0,&help, (fd_set*) 0, &tm);
else
	return select(socket+1,(fd_set*) 0,&help, (fd_set*) 0, NULL);
}

/******************************************************************************/
