#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h> 

#define syserr(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr);
#define syslog(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout);

#endif // __LOG_H__