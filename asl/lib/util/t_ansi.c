#define NO_CFORTRAN

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include "asl.h"


/***************************************************************************
 *
 *	Library to manipulate Ansi Tapes (Version 3)
 *
 *	First try:	13.10.1994	V.H.
 *
 */


/***************************************************************************
 *
 *	Routine  : t_init(path,label)
 *	Arguments: char* path			z.B. /dev/rmt0h
 *		   char* label			Tape label (6 chars)
 *	Result   : int 0			OK
 *		   int errno			errno
 *
 *	Tape will be opened, initialized, rewinded and closed
 */

int t_init(char* path, char* label)
{
int		fd,i;
int		hold_errno;
struct	mtop	mtop;
struct	passwd*	pwd;
char		vol1[81];
					/* open tape device */
if ((fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0666))<0)
	return errno;
					/* rewind tape */
mtop.mt_op	= MTREW;
mtop.mt_count	= 1;
if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
	hold_errno = errno;
	close(fd); 
	return hold_errno;
	}

#if defined(LINUX)
  mtop.mt_op     = MTSETBLK;
  mtop.mt_count  = 0;
  if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
    hold_errno = errno;
    close(fd);
    return hold_errno;
  }
#endif
        
					/* get username */
pwd = getpwuid(getuid());
					/* write volume label */
memset(vol1,' ',80);
strncpy(&vol1[00],"VOL1",4);		/* VOL1 */
strncpy(&vol1[04],label	,6);		/* tape label */
strncpy(&vol1[10]," "	,1);		/* Accessibility Field */
strncpy(&vol1[11],"             ",13);	
strncpy(&vol1[24],"(c) 10/94 VH ",13);
strncpy(&vol1[37],pwd->pw_name,14);	/* user name */
strncpy(&vol1[51],"                            ",28);
strncpy(&vol1[79],"3"	,1);		/* ANSI version 3 */
for(i=0;i<80;i++)			/* set zero to space */
	if (vol1[i]==0) vol1[i]=' ';

if (write(fd,vol1,80)!=80) {
	hold_errno = errno;
	close(fd); 
	return hold_errno;
	}

					/* write EOF */
mtop.mt_op	= MTWEOF;
mtop.mt_count	= 1;
if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
	hold_errno = errno;
	close(fd); 
	return hold_errno;
	}
 
					/* rewind tape */
mtop.mt_op	= MTREW;
mtop.mt_count	= 1;
if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
	hold_errno = errno;
	close(fd); 
	return hold_errno;
	}
 
					/* close file */
close(fd);

return 0;
}

/***************************************************************************
 *
 *	Routine  : t_mount(path)
 *	Arguments: char* path			z.B. /dev/rmt0h
 *	Result   : ANSI_TAPE*			Pointer to structure
 *		   =NULL			error
 *
 *	Tape will be opened, rewind, label read
 */

ANSI_TAPE* t_mount(char* path)
{
ANSI_TAPE	*tape;
int		fd;
FILE*		fp;
struct	mtop	mtop;
char		vol1[81];
char		temp[30];

					/* get memory */
if ((tape=malloc(sizeof(ANSI_TAPE)))==NULL) {
	return NULL;
	}


					/* open tape device */
if ((fd=open(path,O_RDWR|O_CREAT|O_TRUNC,0666))<0) {
        fprintf(stderr,"<I> Tape may be write-locked, try read-only ...\n");
        if ((fd=open(path,O_RDONLY,0666))<0) {
            free(tape);
	    return NULL;
	    }
	}

tape->descriptor = fd;
strncpy(tape->path,path,19);
					/* File pointer */
if ((fp=fdopen(fd,"rw"))==NULL) {
	close(fd);
	free(tape);
	return NULL;
	}
tape->fp = fp;
					/* rewind tape */
mtop.mt_op	= MTREW;
mtop.mt_count	= 1;
if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
	close(fd); 
	free(tape);
	return NULL;
	}

#if defined(LINUX)
  mtop.mt_op     = MTSETBLK;
  mtop.mt_count  = 0;
  if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
    close(fd);
    free(tape);
    return NULL;
  }
#endif
  
if (read(fd,vol1,80)<1) {
	close(fd);
	free(tape);
	return NULL;
	}
					/* checks for VOL1 */
strncpy(temp,&vol1[00],4);
temp[4]=0;
if (strcmp(temp,"VOL1")) {
	close(fd);
	free(tape);
	return NULL;
	}
	
strncpy(tape->label,&vol1[04],6);
tape->label[6]=0;
strncpy(tape->implementation_id,&vol1[24],13);
tape->implementation_id[13]=0;
strncpy(tape->user,&vol1[37],14);
tape->user[14]=0;

switch(vol1[79]) {
	case '3' : tape->version = 3;
		   break;
	case '4' : tape->version = 4;
		   break;
	default  : tape->version = 0;	
	}

					/* save header */
memcpy(tape->vol1,vol1,80);

					/* reset file_position */
tape->status    = 0;
tape->file_pos  = 0;
tape->file_part = T_HEADER;
tape->status   |= T_IGNORE_CASE;

return tape;
}
 
/***************************************************************************
 *
 *	Routine  : t_umount(ANSI_TAPE*)
 *	Arguments: ANSI_TAPE*			Pointer to open tape
 *	Result   : none
 *
 *
 *	Tape will be rewinded and closed
 */

void t_umount(ANSI_TAPE* tape)
{
int	fd;
struct  mtop mtop;

if (!tape) 			/* check if tape in use */
	return;
				/* get file descriptor */
fd = tape->descriptor;

if (tape->status&T_FILE_OPEN)
	t_close(tape);	

if (tape->file_part==T_NEXT_HEADER) {
				/* write 1 EOF to indicate EOT */
	mtop.mt_op	= MTWEOF;
	mtop.mt_count	= 1;
	ioctl(fd,MTIOCTOP, (char*) &mtop); 
	}

				/* rewind or unload tape */
if (tape->status&T_UNLOAD) 
	mtop.mt_op	= MTOFFL;
else
	mtop.mt_op	= MTREW;

mtop.mt_count	= 1;
ioctl(fd,MTIOCTOP, (char*) &mtop); 

close(fd);

free(tape);
tape = NULL;

return;
}

/***************************************************************************
 *
 *	Routine  : t_get_current_entry(ANSI_TAPE*)
 *	Arguments: ANSI_TAPE*			Pointer to mounted tape
 *	Result   : TAPE_ENTRY*			Pointer to structure
 *		   =NULL			error
 *
 *	Current directory entry will be read
 */

TAPE_ENTRY* t_get_current_entry(ANSI_TAPE* tape)
{
TAPE_ENTRY*	entry;
char		hdr1[81];
char		hdr2[81];
char		temp[30];
char		century;
struct	tm	tm;

if (!tape) 				/* check if tape in use */
	return NULL;
					/* get memory */
if ((entry=malloc(sizeof(TAPE_ENTRY)))==NULL)
	return NULL;


memcpy(hdr1,tape->hdr1,80);
memcpy(hdr2,tape->hdr2,80);

strncpy(entry->filename,&hdr1[04],17);	/* filename */
entry->filename[17]=0;
strncpy(entry->label,&hdr1[21],6);	/* tape label */
entry->label[6]=0;
strncpy(temp,&hdr1[27],4);		/* file_section_number */
temp[4]=0;
entry->file_section_number = atoi(temp);
strncpy(temp,&hdr1[31],4);		/* file_sequence_number */
temp[4]=0;
entry->file_sequence_number = atoi(temp);
strncpy(temp,&hdr1[35],4);		/* generation_number */
temp[4]=0;
entry->generation_number = atoi(temp);
strncpy(temp,&hdr1[39],2);		/* generation_version */
temp[2]=0;
entry->generation_version = atoi(temp);
strncpy(entry->creation_date,&hdr1[41],6);	 /* creation date */
entry->creation_date[6]=0; 
strncpy(entry->expiration_date,&hdr1[47],6);	 /* expiration date */ 
entry->expiration_date[6]=0; 
entry->file_security = hdr1[53];	/* file security */
strncpy(temp,&hdr1[54],6);		/* block count */
temp[6]=0;
entry->block_count = atoi(temp);
strncpy(entry->implementation_id,&hdr1[60],13);	/* implementation id */
entry->implementation_id[13]=0;

entry->record_format = hdr2[4];		/* record format */
strncpy(temp,&hdr2[5],5);		/* block length */
temp[5]=0;
entry->block_length = atoi(temp);
strncpy(temp,&hdr2[10],5);		/* record length */
temp[5]=0;
entry->record_length = atoi(temp);

return entry;
}

/***************************************************************************
 *
 *	Routine  : t_get_next_entry(ANSI_TAPE*)
 *	Arguments: ANSI_TAPE*			Pointer to mounted tape
 *	Result   : TAPE_ENTRY*			Pointer to structure
 *		   =NULL			error
 *
 *	Next directory entry will be read
 *	If file sequence number = -1 end of tape
 */

TAPE_ENTRY* t_get_next_entry(ANSI_TAPE* tape)
{
TAPE_ENTRY*	entry;
int		fd;
int		status;
struct	mtop	mtop;
char		hdr1[81];
char		hdr2[81];
char		temp[30];

if (!tape) 			/* check if tape in use */
	return NULL;

				/* File in use ? */
if (tape->status&T_FILE_OPEN)
	return NULL;

fd = tape->descriptor;

				/* get memory */
if ((entry=malloc(sizeof(TAPE_ENTRY)))==NULL)
	return NULL;

				/* set file_pos to HEADER of next file */
if (tape->file_pos) {
	switch(tape->file_part) {
	  case T_NEXT_HEADER :
		entry->file_sequence_number = -1;
		return entry;
		break;
	  case T_HEADER :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 3;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return NULL;
		break;
	  case T_DATA :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 2;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return NULL;
		break;
	  case T_TRAILER :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return NULL;
		break;
	  default :
		free(entry);
	  	return NULL;
	  } /* end switch */
	}

tape->file_part = T_HEADER;

				/* read file header */
status=read(fd,hdr1,80);
if (status==0) {
	entry->file_sequence_number = -1;
	if (tape->file_pos) {
		mtop.mt_op	= MTBSF;
		mtop.mt_count	= 2;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return NULL;
		read(fd,hdr1,80); 
		tape->file_pos++;
		tape->file_part = T_NEXT_HEADER;
		}
	else {
		mtop.mt_op	= MTREW;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return NULL;
		read(fd,hdr1,80); 
		tape->file_pos  = 1;
		tape->file_part = T_NEXT_HEADER;
		}
	return entry;
	}

tape->file_pos++;

if (status<0) {
  	free(entry);
	return NULL;
}
  
if (read(fd,hdr2,80)<0) {
        free(entry);
	return NULL;
}
  

if (!(tape->status&T_USE_HEADER)) {	/* use TRAILER for directory */
	mtop.mt_op	= MTFSF;
	mtop.mt_count	= 2;
	if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) { 
	  	free(entry);
		return NULL;
	}
  	tape->file_part = T_TRAILER;

				/* read file trailer */
	if (read(fd,hdr1,80)<0) {
	  	free(entry);
		return NULL;
	}
  
	if (read(fd,hdr2,80)<0) {
	  	free(entry);
		return NULL;
	}
}

				/* checks for HDR1 or EOF1 */
strncpy(temp,&hdr1[00],4);
temp[4]=0;
if (tape->file_part==T_HEADER)
   if (strcmp(temp,"HDR1")) {
     	free(entry);
	return NULL;
   }
else if (tape->file_part==T_TRAILER)
   if (strcmp(temp,"EOF1")) { 
     	free(entry);
	return NULL;
   }
else {
  	free(entry);
    	return NULL;
}
  
				/* checks for HDR2 or EOF2 */
strncpy(temp,&hdr2[00],4);
temp[4]=0;
if (tape->file_part==T_HEADER)
   if (strcmp(temp,"HDR2")) {
     	free(entry);
	return NULL;
   }
else if (tape->file_part==T_TRAILER)
   if (strcmp(temp,"EOF2")) {
     	free(entry);
	return NULL;
   }
else {
  	free(entry);
	return NULL;
}
  

memcpy(tape->hdr1,hdr1,80);
memcpy(tape->hdr2,hdr2,80);

free(entry);

return t_get_current_entry(tape);
}

/***************************************************************************
 *
 *	Routine  : t_open(ANSI_TAPE*,int,TAPE_ENTRY*)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *		   int	      	mode		T_WRITE,T_READ
 *		   TAPE_ENTRY* 	entry		File description
 *	Result   : int				0  = ok
 *		   				-1 = fail
 *
 *	READ  : Go to begin of data OF current file if entry=NULL
 *		otherwise search file named in entry
 *	WRITE : Write header AFTER current file, go to begin of data
 */

int t_open(ANSI_TAPE* tape,int mode, TAPE_ENTRY* entry)
{
int		fd,i;
int		current_time;
struct 	mtop 	mtop;
struct  tm	tm;
char		hdr1[81];
char		hdr2[81];
char		temp[30];
TAPE_ENTRY*	help_entry;
int		save_status;
char		helpname_1[40];
char		helpname_2[40];
  
  
if (!tape) 			/* check if tape in use */
	return -100;

fd = tape->descriptor;

if (tape->status&T_FILE_OPEN)
	return -1;

if (mode&T_READ) {
	if (entry) {
	        sprintf(temp,"%-17s",entry->filename);
	        strcpy(entry->filename,temp);
		help_entry = t_get_current_entry(tape);
	        while ( mystrcmp(help_entry->filename,entry->filename,
				  tape->status) &&
			(help_entry->file_sequence_number != -1 ) ) {
				free(help_entry);
				help_entry = t_get_next_entry(tape);
				}
		if (help_entry->file_sequence_number==-1) {
			*entry = *help_entry;
			return 0;
			}
		}
	if (!tape->file_pos) 
		free(t_get_next_entry(tape));
	switch(tape->file_part) {
	  case T_NEXT_HEADER :
		return -21;
	  case T_HEADER :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -22;
		break;
	  case T_DATA :
		mtop.mt_op	= MTBSF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -23;
		read(fd,hdr1,80);
		break;
	  case T_TRAILER :
		mtop.mt_op	= MTBSF;
		mtop.mt_count	= 2;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -24;
		read(fd,hdr1,80);
		break;
	  default :
		return -25;
	  }
	tape->file_part = T_DATA;
	tape->status   |= T_FILE_OPEN;
	tape->status   |= T_READ;
	return 0;
	} /* end entry == NULL */
else if (mode&T_WRITE) {
        if (tape->file_pos) {
	  switch(tape->file_part) {
	    case T_NEXT_HEADER :
		tape->file_pos--;
		break;
	    case T_HEADER :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 3;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -30;
		break;
	    case T_DATA :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 2;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -31;
		break;
	    case T_TRAILER :
		mtop.mt_op	= MTFSF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -32;
		break;
	    default :
		return -33;
	  }
	}
  	else {
  	  mtop.mt_op    = MTSEOD;
	  mtop.mt_count = 1;
	  if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) {
	    return -130;
	  }

          mtop.mt_op          = MTBSF;
          mtop.mt_count       = 1;
          if (ioctl(tape->descriptor,MTIOCTOP, (char*) &mtop)<0) {
	    return -131;
	  }
	}

	tape->file_pos++;
	tape->file_part	= T_HEADER;
	
				/* Build HDR1 */
	current_time = time(NULL);
	tm = *gmtime( (time_t*) &current_time);	

	memset(hdr1,' ',80);
	strncpy(&hdr1[00],"HDR1",4);
	strncpy(&hdr1[04],entry->filename,17);
	strncpy(&hdr1[21],tape->label,6);
	strncpy(&hdr1[27],"0001",4);
	  sprintf(temp,"%4d",tape->file_pos);
	  for(i=0;i<4;i++)
		if (temp[i]==' ') temp[i]='0';
	strncpy(&hdr1[31],temp,4);
	strncpy(&hdr1[35],"0001",4);
	strncpy(&hdr1[39],"00",2);
	  sprintf(temp," %2d%3d",tm.tm_year,tm.tm_yday+1);
          for(i=1;i<6;i++)
        	if (temp[i]==' ') temp[i]='0';
	strncpy(&hdr1[41],temp,6);
	strncpy(&hdr1[47]," 99366",6);
	strncpy(&hdr1[53]," 000000",7);
	strncpy(&hdr1[60],"(c) 10/94 VH ",13);
	for(i=0;i<80;i++)
		if (hdr1[i]==0) hdr1[i]=' ';

	memset(hdr2,' ',80);
	strncpy(&hdr2[00],"HDR2",4);
	hdr2[04] = entry->record_format;
	  sprintf(temp,"%5d",entry->block_length);
	  for(i=0;i<5;i++)
		if (temp[i]==' ') temp[i]='0';
	strncpy(&hdr2[05],temp,5);
	  sprintf(temp,"%5d",entry->record_length);
	  for(i=0;i<5;i++)
		if (temp[i]==' ') temp[i]='0';
	strncpy(&hdr2[10],temp,5);
	for(i=0;i<80;i++)
		if (hdr1[i]==0) hdr2[i]=' ';

	if (write(fd,hdr1,80)!=80)
		return -34;
	if (write(fd,hdr2,80)!=80)
		return -35;

	memcpy(tape->hdr1,hdr1,80);
	memcpy(tape->hdr2,hdr2,80);

	mtop.mt_op	= MTWEOF;
	mtop.mt_count	= 1;
	if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
		return -36;

	if ((help_entry=t_get_current_entry(tape))==NULL) 
		return -37;
		
	*entry = *help_entry;

	tape->file_part = T_DATA;

	tape->block_count = 0;
	tape->status     |= T_FILE_OPEN;
	tape->status     |= T_WRITE;

	return 0;
	}

return -40;
}


/***************************************************************************
 *
 *	Routine  : t_close(ANSI_TAPE*)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *	Result   : int				0  = ok
 *		   				-1 = fail
 *	closes file
 *
 */

int t_close(ANSI_TAPE* tape)
{
int		fd,i;
struct 	mtop 	mtop;
char		hdr1[81];
char		hdr2[81];
char		temp[30];

if (!tape) 			/* check if tape in use */
	return -100;

fd = tape->descriptor;

if (!(tape->status&T_FILE_OPEN))
	return -1;

if (tape->status&T_READ) {
	tape->status   &= (~T_FILE_OPEN);
	tape->status   &= (~T_READ);
	return 0;
	}
else if (tape->status&T_WRITE) {
	mtop.mt_op	= MTWEOF;
	mtop.mt_count	= 1;
	if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
		return -10;

	memcpy(hdr1,tape->hdr1,80);
	memcpy(hdr2,tape->hdr2,80);

	strncpy(&hdr1[00],"EOF1",4);
	  sprintf(temp,"%6d",tape->block_count);
          for(i=0;i<6;i++)
        	if (temp[i]==' ') temp[i]='0';
	strncpy(&hdr1[54],temp,6);

	strncpy(&hdr2[00],"EOF2",4);

	if (write(fd,hdr1,80)!=80)
		return -11;
	if (write(fd,hdr2,80)!=80)
		return -12;

	if (tape->status&T_WRITE_CONT) {
		mtop.mt_op	= MTWEOF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -13;
		}
	else {
		mtop.mt_op	= MTWEOF;
		mtop.mt_count	= 2;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -14;

		mtop.mt_op	= MTBSF;
		mtop.mt_count	= 1;
		if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
			return -15;
		}

	tape->file_pos++;
	tape->file_part = T_NEXT_HEADER;

	tape->status   &= (~T_FILE_OPEN);
	tape->status   &= (~T_WRITE);

	return 0;
	}

return -20;
}

/***************************************************************************
 *
 *	Routine  : t_read(ANSI_TAPE*,char* buffer,int)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *		   char*	buffer		Pointer to memory
 *		   int	      	n		Number of Bytes to read
 *	Result   : int				Number of Bytes read
 *
 */

int t_read(ANSI_TAPE* tape, char* buffer, int n)
{
int	fd;
int 	status;

if (!tape) 			/* check if tape in use */
	return -100;

fd = tape->descriptor;

if ( (tape->status&T_FILE_OPEN) && (tape->status&T_READ) 
     && (tape->file_part==T_DATA) ) {
	status = read(fd,buffer,n);
	if (status==0)
		tape->file_part = T_TRAILER;
	return status;
	}
else
	return -1;
}

int t_fread(ANSI_TAPE* tape, char* buffer, int n)
{
FILE*	fp;
int 	status;

if (!tape) 			/* check if tape in use */
	return -100;

fp = tape->fp;

if ( (tape->status&T_FILE_OPEN) && (tape->status&T_READ) 
     && (tape->file_part==T_DATA) ) {
	status = fread(buffer,1,n,fp);
	if (status==0)
		tape->file_part = T_TRAILER;
	return status;
	}
else
	return -1;
}


/***************************************************************************
 *
 *	Routine  : t_write(ANSI_TAPE*,char* buffer,int)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *		   char*	buffer		Pointer to memory
 *		   int	      	n		Number of Bytes to read
 *	Result   : int				Number of Bytes read
 *
 */

int t_write(ANSI_TAPE* tape, char* buffer, int n)
{
int	fd;

if (!tape) 			/* check if tape in use */
	return -100;

fd = tape->descriptor;

if ( (tape->status&T_FILE_OPEN) && (tape->status&T_WRITE) ) {
	tape->block_count++;
	return (write(fd,buffer,n));
	}
else
	return -1;
}

/***************************************************************************
 *
 *	Routine  : t_rewind(ANSI_TAPE*)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *	Result   : int				<0 = fails
 *
 */

int t_rewind(ANSI_TAPE* tape)
{
int 	fd;
char	dummy[81];
struct  mtop mtop;

if (!tape) 			/* check if tape in use */
	return -100;

if (tape->status & T_FILE_OPEN) 
		return -1;

fd = tape->descriptor;

mtop.mt_op	= MTREW;
mtop.mt_count	= 1;
if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
		return -2;

read(fd,dummy,80);

tape->file_pos  = 0;
tape->file_part = T_HEADER;
}

/***************************************************************************
 *
 *	Routine  : t_goto(ANSI_TAPE*,int)
 *	Arguments: ANSI_TAPE* 	tape		Pointer to mounted tape
 *		   int		filenumber
 *	Result   : int				<0 = fails
 *
 */

int t_goto(ANSI_TAPE* tape, int filenumber)
{
int 	fd;
char	dummy[81];
int	eof_diff;
int	file_now, part_now;
struct  mtop mtop;

if (!tape) 			/* check if tape in use */
	return -100;

if (tape->status & T_FILE_OPEN) 
	return -1;

fd = tape->descriptor;

if (filenumber<2) {		/* go to beginning of tape */
    t_rewind(tape);
    return 0;
    }

file_now = tape->file_pos;
part_now = tape->file_part;

if (file_now==0) {		/* manage label as file 1 */
	file_now = 1;
	part_now = T_HEADER;
	}

if (file_now<filenumber) {
	eof_diff = 3*(filenumber-file_now);
	switch (part_now) {
	    case T_NEXT_HEADER :
		return -2;
		break;
	    case T_HEADER :
		break;
	    case T_DATA :
		eof_diff -= 1;
		break;
	    case T_TRAILER :
		eof_diff -= 2;
		break;
	    default :
		return -3;
	    }

    	mtop.mt_op		= MTFSF;
    	mtop.mt_count	= eof_diff;
    	if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
		return -4;

    	tape->file_pos  = filenumber;
    	tape->file_part = T_HEADER;

    	return 0;
    	}
else if (file_now>filenumber) {
	eof_diff = 3*(filenumber-file_now) + 1;
	switch (part_now) {
	    case T_NEXT_HEADER :
	    case T_HEADER :
		break;
	    case T_DATA :
		eof_diff += 1;
		break;
	    case T_TRAILER :
		eof_diff += 2;
		break;
	    default :
		return -5;
	    }

    	mtop.mt_op		= MTBSF;
    	mtop.mt_count		= eof_diff;
    	if (ioctl(fd,MTIOCTOP, (char*) &mtop)<0) 
		return -6;

	read(fd,dummy,80);

    	tape->file_pos  = filenumber;
    	tape->file_part = T_HEADER;

    	return 0;
    	}

return 0;
}

/***************************************************************************
 *
 *	Routine  : mystrcmp(char* s1, char* s2, int status)
 *	Arguments: char*	s1,s2	Strings to compare
 *	Result   : int			0/1 like strcmp
 *
 * 	Looks for T_IGNORE_CASE
 */

int mystrcmp(char* s1, char* s2, int status)
{
  if (status & T_IGNORE_CASE)
    return strcasecmp(s1,s2);
  else
    return strcmp(s1,s2);
}

    
