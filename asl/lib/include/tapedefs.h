#ifndef __TAPEDEFSHEADER
#define __TAPEDEFSHEADER

#define TAPEUTIL_VERSION "Tape Utility Version 1.0, 1993-97 by VH"

#include <sys/types.h>          /* network access ... */
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>         /* sockets ... */
#include <fcntl.h>
#include <errno.h>              /* Error handling */
#include <sys/ioctl.h>          /* for taping */
#if defined(AIX)
# include <sys/tape.h>
# include <sys/select.h>
#else
# include <sys/mtio.h>
#endif

#if defined(SUN)
# include <sys/stat.h>
# define MTSEOD         MTEOM
#endif

#if defined(LINUX)
//# include <linux/stat.h>
# include <sys/stat.h>		// Why this is so, we don't know!!!!
									// DLH + MR 13.03.03
# define MTSEOD         MTEOM
#endif

#if defined(AIX)
# define MTSEOD		STSEOD
# define MTIOCTOP       STIOCTOP
# define MTWEOF         STWEOF
# define MTFSF          STFSF
# define MTBSF          STRSF
# define MTREW          STREW
# define MTOFFL         STOFFL
# define mtop           stop
# define mt_op          st_op
# define mt_count       st_count
#endif

#define	T_HEADER	0
#define T_DATA		1
#define T_TRAILER	2
#define T_NEXT_HEADER	3

#define T_FILE_OPEN	1
#define T_WRITE		2
#define T_READ		4
#define T_USE_HEADER	8
#define T_UNLOAD	16
#define T_WRITE_CONT	32
#define T_IGNORE_CASE   64

typedef struct {
	int   descriptor;		/* file descriptor to access tape */
	FILE* fp;			/* descriptor got with fdopen() */
	int   status;			/* tape_status */
	char  path[20];			/* tape device */
	char  label[7];			/* tape label */
	int   version;			/* ANSI Version 3/4 */
	int   file_pos;			/* current file number */
	int   file_part;		/* 0=hdr,1=data,2=eof */
	int   block_count;		/* file length in blocks */
	char  implementation_id[14];	
	char  user[15];			/* user name */
	char  vol1[80];
	char  hdr1[80];			/* current file in use */
	char  hdr2[80];
	} ANSI_TAPE;

typedef struct {
	char  filename[18];		/* interchange filename */
	char  label[7];			/* used tape label */
	int   file_section_number;	/* = 1 */
	int   file_sequence_number;	/* 1,2,3,... */
	int   generation_number;	/* = 1 */
	int   generation_version;	/* = 0 */
	char  creation_date[7];
	char  expiration_date[7];	/* = "099366" */
	char  file_security;		/* = " " */
	int   block_count;		/* = 0 */
	char  implementation_id[14];	
	char  record_format;		/* F,D,S */
	int   block_length;
	int   record_length;
	} TAPE_ENTRY;
	

ANSI_TAPE*  t_mount(char*);
TAPE_ENTRY* t_get_next_entry(ANSI_TAPE*);
TAPE_ENTRY* t_get_current_entry(ANSI_TAPE*);
int	    t_init(char*,char*);
void	    t_umount(ANSI_TAPE*);
int	    t_open(ANSI_TAPE*,int,TAPE_ENTRY*);
int	    t_close(ANSI_TAPE*);
int	    t_read(ANSI_TAPE*,char*,int);
int	    t_fread(ANSI_TAPE*,char*,int);
int	    t_write(ANSI_TAPE*,char*,int);
int	    t_rewind(ANSI_TAPE*);
int	    t_goto(ANSI_TAPE*,int);

#define T_get_next_entry(A1,A2) { TAPE_ENTRY* t; \
				  t = t_get_next_entry(A2); \
				  if (t) { A1 = *t; free(t); } \
				  else A1.file_sequence_number = -1; }

#define T_get_current_entry(A1,A2) { TAPE_ENTRY* t; \
				  t = t_get_current_entry(A2); \
				  if (t) { A1 = *t; free(t); } \
				  else A1.file_sequence_number = -1; }

#define writes(A1,A2) write(A1,A2,strlen(A2))

/* From here only for tape utilities */

ssize_t piperead(int,const void*,size_t);
# define t_pread(a,b,c) readpipe(a,b,c)


typedef struct {
  char header[100];
  char buffer[8192];
} tu_control;

#endif
