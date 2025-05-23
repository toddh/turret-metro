#ifndef _GLOBALSH_
#define _GLOBALSH_

#include <RTTStream.h>

#ifdef USE_RTT_STREAM
extern RTTStream _rttstream;
#define console _rttstream
#else
#define console Serial
#endif


#endif

