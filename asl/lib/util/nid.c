/* 
 * $Log: nid.c,v $
 * Revision 1.7  1998/03/25 10:45:32  hejny
 * Added error handling for 2D histograms
 *
 * Revision 1.6  1998/03/03 15:42:04  hejny
 * Error calculation for 1D spectra implemented
 *
 * Revision 1.5  1998/02/16 16:33:32  hejny
 * Bigfix: V2.4B and earlier are readable again
 *
 * Revision 1.4  1998/01/28 17:30:14  hejny
 * Minor Bugfixes
 *
 * Revision 1.3  1998/01/28 17:26:21  hejny
 * Adding profile histograms to the analysis.
 *
 * Revision 1.2  1998/01/28 14:50:38  hejny
 * Added _Id_ and _Log_
 *
 */

static char *rcsid="$Id: nid.c,v 1.7 1998/03/25 10:45:32 hejny Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <err.h>
#include "asl.h"

#if defined(OSF1) || defined(ULTRIX)
//# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

histo hlist[MAX_HISTO];
int   hcount = { 0 };
pict  plist[MAX_PICTURE];
int   pcount = { 0 };
hist_array alist[MAX_ARRAY];
int   acount = { 0 };

int   requested_pawspace = { 0 };

#ifdef WARPFILL
  int   id_addr[4*MAX_HISTO];
#endif

void  conv_v2_6E(histo_v2_6E* );

/****************************************************************************/

int assign_id_new(char* name,int flag)
{
int i , found;
#if defined(VMS)
 int 	a1,a3,a6;
 float	a4,a5,a7,a8,a9;
 char   a2[80];
#endif

found = -1;
for(i=0;i<hcount;i++)
  if (!strcmp(hlist[i].histname,name))
    found = i;

if (found<0) return 0;

if ( (hlist[found].nx) && (hlist[found].ny) ) {
  if (hlist[found].flags & HF_PROFILE) {
    if (flag)
	 printf("    Booking Profile Histogram %s\n",hlist[found].histname);
#if defined(OSF1)
    HBPROF(hlist[found].id,
	   hlist[found].title,
	   hlist[found].nx,
	   hlist[found].xmin,
	   hlist[found].xmax,
	   hlist[found].ymin,
	   hlist[found].ymax,
	   "S");
#elif defined(VMS)
    a1 = hlist[found].id;
    strcpy(a2,hlist[found].title);
    a3 = hlist[found].nx;
    a4 = hlist[found].xmin;
    a5 = hlist[found].xmax;
    a7 = hlist[found].ymin;
    a8 = hlist[found].ymax;
    HBPROF(a1,a2,a3,a4,a5,a7,a8,"S");
#endif
    requested_pawspace += hlist[found].nx;
  }
  else {
    if (flag)
	 printf("    Booking 2-Dim Spektrum %s\n",hlist[found].histname);
#if defined(OSF1)
    HBOOK2(hlist[found].id,
	   hlist[found].title,
	   hlist[found].nx,
	   hlist[found].xmin,
	   hlist[found].xmax,
	   hlist[found].ny,
	   hlist[found].ymin,
	   hlist[found].ymax,
	   hlist[found].packing);
#elif defined(VMS)
    a1 = hlist[found].id;
    strcpy(a2,hlist[found].title);
    a3 = hlist[found].nx;
    a4 = hlist[found].xmin;
    a5 = hlist[found].xmax;
    a6 = hlist[found].ny;
    a7 = hlist[found].ymin;
    a8 = hlist[found].ymax;
    a9 = hlist[found].packing;
    HBOOK2(a1,a2,a3,a4,a5,a6,a7,a8,a9);
#endif
    if (hlist[found].flags & HF_ERROR) {
      HBAR2(hlist[found].id); 
      requested_pawspace += hlist[found].nx*hlist[found].ny;
    }
    requested_pawspace += hlist[found].nx*hlist[found].ny;
  }
}
else if (hlist[found].nx) {
  if (flag)
     printf("    Booking 1-Dim Spektrum %s\n",hlist[found].histname);
  HBOOK1( hlist[found].id,
	  hlist[found].title,
	  hlist[found].nx,
	  hlist[found].xmin,
	  hlist[found].xmax,
	  hlist[found].packing);
  if (hlist[found].flags & HF_ERROR) {
    HBARX(hlist[found].id); 
    requested_pawspace += hlist[found].nx;
  }
  requested_pawspace += hlist[found].nx;
  }
else
  return 0;

return hlist[found].id;
}

/****************************************************************************/

int assign_array(char* name,int* array)
{
int i , found;
int chx;
char hname[80];

found = -1;
for(i=0;i<acount;i++)
  if (!strcmp(alist[i].basename,name))
    found = i;

if (found<0) return 0;

printf("    Booking ARRAY [%3d,%3d] %s\n"
		,alist[found].lowx,alist[found].highx,alist[found].basename);
for(chx=alist[found].lowx;chx<=alist[found].highx;chx++) { 
  sprintf(hname,"%s_%d",name,chx);
  array[chx] = assign_id_new(hname,0);
  }

alist[found].baseid = array[alist[found].lowx];
  
return 1;
}

/****************************************************************************/

int read_h_entry(char* name, FILE* fp)
{
char line[100],key[100];
char *pos;
int   loop;

char  title[80];
char  x_axis[80], y_axis[80];
int   nx,ny;
float xmin,xmax,ymin,ymax,packing;

char  realname[80];
char  temp[40];
int   lowx,highx,flag;
int   chx,old_hcount;
int   protect, h_flags;
  
  
title[0] = 0;
x_axis[0] = 0;
y_axis[0] = 0;
nx = 100; 
xmin = 0.;
xmax = 1.;
ny = 0;
ymin = ymax = 0.0;
packing = 0;
h_flags = 0;
  
protect = 0;
flag = 0;
lowx = highx = 0;

loop = 1;

strcpy(realname,name);
if (pos = strstr(realname,"[")) {
  *pos = 0;
  strcpy(temp,pos+1);
  *strstr(temp,",") = ' ';
  *strstr(temp,"]") = ' ';
  sscanf(temp,"%d %d",&lowx,&highx);
  flag = 1;
  }

while(loop) {
  if (fgets(line, sizeof(line), fp)) {
    sscanf(line,"%s",key);
    if (!strcmp(key,"TITLE")) {
	pos = strstr(line,"TITLE") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(title,pos);
	pos = strstr(title,"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"XAXIS")) {
	pos = strstr(line,"XAXIS") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(x_axis,pos);
	pos = strstr(x_axis,"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"YAXIS")) {
	pos = strstr(line,"YAXIS") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(y_axis,pos);
	pos = strstr(y_axis,"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"DEFX")) {
	pos = strstr(line,"DEFX") + 4;
	sscanf(pos,"%d %f %f",&nx,&xmin,&xmax);
	}
    if (!strcmp(key,"DEFY")) {
	pos = strstr(line,"DEFY") + 4;
	sscanf(pos,"%d %f %f",&ny,&ymin,&ymax);
	}
    if (!strcmp(key,"PACK")) {
	pos = strstr(line,"PACK") + 4;
	sscanf(pos,"%f",&packing);
	}
    if (!strcmp(key,"PROT")) {
	pos = strstr(line,"PROT") + 4;
	sscanf(pos,"%d",&protect);
	}
    if (!strcmp(key,"PROFILE")) {
	h_flags |= HF_PROFILE;
	}
    if (!strcmp(key,"ERROR")) {
	h_flags |= HF_ERROR;
	}
    if (!strcmp(key,"HEND")) {
	loop = 0;
	}
    }
  else
    loop = 0;
  }

if (flag) {
				/* enough space ? */
  if ( (acount+1) == MAX_ARRAY) 
    return -1;

  strcpy(alist[acount].basename, realname);
  alist[acount].lowx	  = lowx;
  alist[acount].highx	  = highx;

  old_hcount = hcount;
  for(chx=lowx;chx<=highx;chx++) {
				/* enough space ? */
    if ( (hcount+1) == MAX_HISTO) {
	  hcount = old_hcount;
   	  return -1;
	  }
    sprintf(hlist[hcount].histname,"%s_%d",realname,chx);
    sprintf(hlist[hcount].title,title,chx);
    strcpy(hlist[hcount].x_axis, x_axis);
    strcpy(hlist[hcount].y_axis, y_axis);
    hlist[hcount].id      = hcount+1 + MAX_HISTO;
    if (protect) hlist[hcount].id = -hlist[hcount].id;
    hlist[hcount].nx      = nx;
    hlist[hcount].xmin    = xmin;
    hlist[hcount].xmax    = xmax;
    hlist[hcount].ny      = ny;
    hlist[hcount].ymax    = ymax;
    hlist[hcount].ymin    = ymin;
    hlist[hcount].packing = packing;
    hlist[hcount].flags   = h_flags;
    
    hcount++;
    } /* end chx */
  acount++;
  return 0;
  }
				/* enough space ? */
if ( (hcount+1) == MAX_HISTO) 
  return -1;

strcpy(hlist[hcount].histname, name);
strcpy(hlist[hcount].title, title );
strcpy(hlist[hcount].x_axis, x_axis);
strcpy(hlist[hcount].y_axis, y_axis);
hlist[hcount].id      = hcount+1;
if (protect) hlist[hcount].id = -hlist[hcount].id;
hlist[hcount].nx      = nx;
hlist[hcount].xmin    = xmin;
hlist[hcount].xmax    = xmax;
hlist[hcount].ny      = ny;
hlist[hcount].ymax    = ymax;
hlist[hcount].ymin    = ymin;
hlist[hcount].packing = packing;
hlist[hcount].flags   = h_flags;

hcount++;
return 0;

}

/****************************************************************************/

int read_p_entry(char* name, FILE* fp)
{
char line[100],key[100];
char *pos;
int  loop,i;

char  title[80];
char  histline[32][120], zoneinfo[32][20];
int   hnum = 0;
int   zone_x, zone_y;
float size_x, size_y;

title[0] = 0;
zone_x = 1;
zone_y = 1;
size_x = 21.0;
size_y = 21.0;

loop = 1;

while(loop) {
  if (fgets(line, sizeof(line), fp)) {
    sscanf(line,"%s",key);
    if (!strcmp(key,"TITLE")) {
	pos = strstr(line,"TITLE") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(title,pos);
	pos = strstr(title,"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"HLINE")) {
	pos = strstr(line,"HLINE") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(histline[hnum++],pos);
	pos = strstr(histline[hnum - 1],"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"ZINFO")) {
	pos = strstr(line,"ZINFO") + 5;
	while ( (*pos == ' ') || (*pos == '\t') ) pos++;
	strcpy(zoneinfo[hnum - 1],pos);
	pos = strstr(zoneinfo[hnum - 1],"\n");
	if (pos) *pos = 0;
	}
    if (!strcmp(key,"SIZE")) {
	pos = strstr(line,"SIZE") + 4;
	sscanf(pos,"%f %f",&size_x, &size_y);
	}
    if (!strcmp(key,"ZONE")) {
	pos = strstr(line,"ZONE") + 4;
	sscanf(pos,"%d %d",&zone_x, &zone_y);
	}
    if (!strcmp(key,"PEND")) {
	loop = 0;
	}
    }
  else
    loop = 0;
  }
				/* enough space ? */
if ( (pcount+1) == MAX_PICTURE) 
  return -1;

strcpy(plist[pcount].pictname, name);
strcpy(plist[pcount].title, title );
plist[pcount].size_x = size_x;
plist[pcount].size_y = size_y;
plist[pcount].zone_x = zone_x;
plist[pcount].zone_y = zone_y;
plist[pcount].hnum   = hnum;

for(i=0;i<hnum;i++) {
  strcpy(plist[pcount].histline[i],histline[i]);
  strcpy(plist[pcount].zoneinfo[i],zoneinfo[i]);
  }

pcount++;
return 0;

}

/****************************************************************************/

histo*	get_h_entry(char* name)
{
int i;

for(i=0;i<hcount;i++)
  if (!strcmp(hlist[i].histname,name))
    return &hlist[i];

return NULL;
}

/****************************************************************************/

pict*	get_p_entry(char* name)
{
int i;

for(i=0;i<pcount;i++)
  if (!strcmp(plist[i].pictname,name))
    return &plist[i];

return NULL;
}

/****************************************************************************/

int save_nid_structure(char* filename)
{
  int fdes;
  int tmp;
  
				       /* open logfile */
  if ((fdes=open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644))<0) {
	return -1;
  }

  write(fdes,(char*) "V2.7B", 5);
  tmp = MAX_HISTO;
  write(fdes,(char*) &tmp,    sizeof(tmp)); 
  tmp = MAX_PICTURE;
  write(fdes,(char*) &tmp,    sizeof(tmp)); 
  tmp = MAX_ARRAY;
  write(fdes,(char*) &tmp,    sizeof(tmp)); 
  write(fdes,(char*) &hcount, sizeof(hcount));
  write(fdes,(char*) &pcount, sizeof(pcount));
  write(fdes,(char*) &acount, sizeof(acount));
  write(fdes,(char*) hlist  , sizeof(hlist));
  write(fdes,(char*) plist  , sizeof(plist));
  write(fdes,(char*) alist  , sizeof(alist));

  close(fdes);
  return 0;
}

/****************************************************************************/

int load_nid_structure(char* filename)
{
  int fdes;
  int tmp_max_histo;
  int tmp_max_picture;
  int tmp_max_array;
  int check;

  histo_v2_6E*  hlist_v2_6E;
  
  char version[6];

				       /* open logfile */
  if ((fdes=open(filename, O_RDONLY, 0644))<0) {
	return -1;
  }
  
  
  read(fdes,(char*) version, 5);
  version[5]=0;
  
  if (!strcmp(version,"V2.4B")) {
    printf("    HLIST File Version V2.4B\n");
    hlist_v2_6E = (histo_v2_6E*) malloc(sizeof(histo_v2_6E)*MAX_HISTO);
    read(fdes,(char*) &hcount,     sizeof(hcount));
    read(fdes,(char*) &pcount,     sizeof(pcount));
    read(fdes,(char*) &acount,     sizeof(acount));
    read(fdes,(char*) hlist_v2_6E, sizeof(histo_v2_6E)*MAX_HISTO);
    read(fdes,(char*) plist  ,     sizeof(plist));
    read(fdes,(char*) alist  ,     sizeof(alist));
    conv_v2_6E(hlist_v2_6E);
    free(hlist_v2_6E);
  }
  else if (!strcmp(version,"V2.6E")) {
    printf("    HLIST File Version V2.6E\n");
    read(fdes,(char*) &tmp_max_histo  , sizeof(tmp_max_histo));
    read(fdes,(char*) &tmp_max_picture, sizeof(tmp_max_picture));
    read(fdes,(char*) &tmp_max_array  , sizeof(tmp_max_array));
    
    check=0;

    if (tmp_max_histo>MAX_HISTO) {
      printf("<E> Used MAX_HISTO value (%d) > current (%d)\n",
	     tmp_max_histo, MAX_HISTO);
      check = 1;
    }
    if (tmp_max_picture>MAX_PICTURE) {
      printf("<E> Used MAX_PICTURE value (%d) > current (%d)\n",
	     tmp_max_picture, MAX_PICTURE);
      check = 1;
    }
    if (tmp_max_array>MAX_ARRAY) {
      printf("<E> Used MAX_ARRAY value (%d) > current (%d)\n",
	     tmp_max_histo, MAX_ARRAY);
      check = 1;
    }
    if (check==1) {
      printf("    Change these values in asl.h and recompile !\n");
      printf("    Delete Analysis from Server !\n");
      exit(1);
    }

    hlist_v2_6E = (histo_v2_6E*) malloc(sizeof(histo_v2_6E)*tmp_max_histo);
    read(fdes,(char*) &hcount, sizeof(hcount));
    read(fdes,(char*) &pcount, sizeof(pcount));
    read(fdes,(char*) &acount, sizeof(acount));
    read(fdes,(char*) hlist_v2_6E, tmp_max_histo*sizeof(histo_v2_6E));
    read(fdes,(char*) plist,       tmp_max_picture*sizeof(pict));
    read(fdes,(char*) alist,       tmp_max_array*sizeof(hist_array));
    conv_v2_6E(hlist_v2_6E);
    free(hlist_v2_6E);
  }
  else if (!strcmp(version,"V2.7B")) {
    printf("    HLIST File Version V2.7B\n");
    read(fdes,(char*) &tmp_max_histo  , sizeof(tmp_max_histo));
    read(fdes,(char*) &tmp_max_picture, sizeof(tmp_max_picture));
    read(fdes,(char*) &tmp_max_array  , sizeof(tmp_max_array));
    
    check=0;

    if (tmp_max_histo>MAX_HISTO) {
      printf("<E> Used MAX_HISTO value (%d) > current (%d)\n",
	     tmp_max_histo, MAX_HISTO);
      check = 1;
    }
    if (tmp_max_picture>MAX_PICTURE) {
      printf("<E> Used MAX_PICTURE value (%d) > current (%d)\n",
	     tmp_max_picture, MAX_PICTURE);
      check = 1;
    }
    if (tmp_max_array>MAX_ARRAY) {
      printf("<E> Used MAX_ARRAY value (%d) > current (%d)\n",
	     tmp_max_histo, MAX_ARRAY);
      check = 1;
    }
    if (check==1) {
      printf("    Change these values in asl.h and recompile !\n");
      printf("    Delete Analysis from Server !\n");
      exit(1);
    }
          
    read(fdes,(char*) &hcount, sizeof(hcount));
    read(fdes,(char*) &pcount, sizeof(pcount));
    read(fdes,(char*) &acount, sizeof(acount));
    read(fdes,(char*) hlist, tmp_max_histo*sizeof(histo));
    read(fdes,(char*) plist, tmp_max_picture*sizeof(pict));
    read(fdes,(char*) alist, tmp_max_array*sizeof(hist_array));
  }
  else {
    int i;
    hist_array_old temp[MAX_ARRAY];
    char hname[80];
    histo* help;
    
    printf("    No HLIST File Version\n");
				       /* close histfile */
    close(fdes);
				       /* open histfile again */
    if ((fdes=open(filename, O_RDONLY, 0644))<0) {
      return -1;
    }
  
    hlist_v2_6E = (histo_v2_6E*) malloc(sizeof(histo_v2_6E)*MAX_HISTO);
    read(fdes,(char*) &hcount, sizeof(hcount));
    read(fdes,(char*) &pcount, sizeof(pcount));
    read(fdes,(char*) &acount, sizeof(acount));
    read(fdes,(char*) hlist_v2_6E, sizeof(histo_v2_6E)*MAX_HISTO);
    read(fdes,(char*) plist  ,     sizeof(plist));
    read(fdes,(char*) temp   ,     sizeof(temp));
    conv_v2_6E(hlist_v2_6E);
    free(hlist_v2_6E);
    
    for(i=0;i<acount;i++) {
      sprintf(hname,"%s_%d",temp[i].basename,temp[i].lowx);
      help = get_h_entry(hname);
      strcpy(alist[i].basename,temp[i].basename);
      alist[i].lowx   = temp[i].lowx;
      alist[i].highx  = temp[i].highx;
      if (help)
	alist[i].baseid = help->id;
      else 
	alist[i].baseid = 0;
    }
  }
  
  close(fdes);
  return 0;
}

/****************************************************************************/

void nid_reset_all()
{
  int i,id;

  for(i=0;i<hcount;i++) {
    id = hlist[i].id;
    if ( (id>0) && HEXIST(id) )	HRESET(id," ");
  }
  
return;
}

  
/****************************************************************************/

int get_h_list(char* name, histo** hl)
{
int i;
int counter = 0;

for(i=0;i<hcount;i++)
  if (ku_match(hlist[i].histname,name,1))
    hl[counter++] = &hlist[i];

return counter;
}

/****************************************************************************/

int	get_p_list(char* name, pict** pl)
{
int i;
int counter = 0;

for(i=0;i<pcount;i++)
  if (ku_match(plist[i].pictname,name,1))
    pl[counter++] = &plist[i];

return counter;
}

/****************************************************************************/

void	conv_v2_6E(histo_v2_6E* alt)
{
  int i;
  
  for(i=0;i<hcount;i++) {
    strcpy(hlist[i].histname, alt[i].histname);
    strcpy(hlist[i].x_axis,   alt[i].x_axis);
    strcpy(hlist[i].y_axis,   alt[i].y_axis);
    strcpy(hlist[i].title,    alt[i].title);
    hlist[i].id      = alt[i].id;
    hlist[i].nx      = alt[i].nx;
    hlist[i].xmin    = alt[i].xmin;
    hlist[i].xmax    = alt[i].xmax;
    hlist[i].ny      = alt[i].ny;
    hlist[i].ymin    = alt[i].ymin;
    hlist[i].ymax    = alt[i].ymax;
    hlist[i].packing = alt[i].packing;
    hlist[i].flags   = 0;
  }
  
  return;
}
  
/****************************************************************************/

void enable_error(int id)
{
  int found = 0;
  int i;

  for(i=0;i<hcount;i++) if (hlist[i].id == id) found=i;
  
  if ((found == 0) || 
      (hlist[found].flags & HF_ERROR))	return;
  
  if (hlist[found].ny==0) {
    HBARX(hlist[found].id); 
  }
  else {
    HBAR2(hlist[found].id);
  }
  hlist[found].flags |= HF_ERROR;
  
  return;
}

/****************************************************************************/

