#ifndef __LOG_H__
#define __LOG_H__

#include "../AeroPlatform.h"

#if defined(AERO_PLATFORM_WINDOWS)
	#define syserr(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } while(0)
	#define syslog(...) do { fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); } while(0)
#elif defined(AERO_PLATFORM_LINUX)
	#define syserr(...) do { fprintf(stderr, ##__VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } while(0)
	#define syslog(...) do { fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); } while(0)
#endif

#endif // __LOG_H__
