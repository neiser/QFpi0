#if defined(OSF1) && !defined(SUN)
# include "cfortran_ansi.h"
#elif defined(VMS) || defined(ULTRIX) || defined(SUN)
# include "cfortran.h"
# define KUCLOS(A1) CCALLSFSUB1(KUCLOS,kuclos,INT,A1)
#endif

# include "cernlib.h"

#define CONTROL() 	CCALLSFSUB0(CONTROL,control)
#define UTILITY() 	CCALLSFSUB0(UTILITY,utility)

#define SQR(x)		((x)*(x))
