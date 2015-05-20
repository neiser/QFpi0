#include <stdio.h>
#include <stdlib.h>

#if defined(OSF1) || defined(ULTRIX)
# include <strings.h>
#elif defined(VMS)
# include <string.h>
#endif

int rand();

/************************************************************************/

float get_random(float lower,float upper)
{
float range;

range = upper - lower;
return ((float) rand()/(float) RAND_MAX)*range + lower;

}

/************************************************************************/

