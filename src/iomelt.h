#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define IOFILE "_IoFile.out"
#define FILESIZE (10*1024*1024)
#define BLOCKSIZE (4*1024)
#define KILO 1024
#define MEGA (KILO*1024)
#define GIGA (MEGA*1024)
#define NUM_TESTS 5

/* generic include files */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* for open() and related IO functions */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* for struct timeval (used in rusage) */
#include <sys/time.h>
/* for clock_gettime */
#include <time.h>
/* for getrusage() */
#include <sys/resource.h>
/* for memset() */
#include <string.h>
/* for bool type */
#include <stdbool.h>
/* signal handling */
#include <signal.h>
/* feature test macros */
#ifndef __APPLE__
#include <features.h>
#endif
#include <stdarg.h> /* for variable argument lists */
#include <errno.h>
#include <sys/statvfs.h>
#include <ctype.h>

typedef struct {
	char device[256];  /* e.g. /dev/nvme0n1 or server:/export */
	char model[256];   /* drive model, or "Remote filesystem"  */
	char vendor[256];  /* manufacturer                         */
	char type[32];     /* "HDD", "SSD", "NVMe", "nfs", etc.   */
	char fstype[64];   /* raw filesystem type from the OS      */
	int  is_remote;    /* 1 if network filesystem              */
} driveInfo;

typedef struct {
	char testName[128];
	unsigned int totalCalls;
	unsigned long blocksIn;
	unsigned long blocksOut;
	double wallClockTime;
	double userTime;
	double systemTime;
} ioMetrics;


void printHelp(void);
void startUsage(void);
void resetUsage(void);
ioMetrics getUsage(void);
double getTime(void);
double getDelta(double);
void bytesToHuman(char *,  double);
void sigint_handler(int);
unsigned long int parseFileSize(char *);
void myWarn(short int, const char *, const char *, ...);
int myOpen(const char *, const bool);
unsigned long int roundUpFileSize(const unsigned long int, const unsigned int);
int isPowerOfTwo(unsigned int);
unsigned int getBlockSize(void);
off_t myRandomSeek(int, unsigned long, int);
ioMetrics serialWrite(int,unsigned long, int, char *);
ioMetrics serialRead(int,unsigned long, int, char *);
ioMetrics randomRewrite(int,unsigned long, int, char *);
ioMetrics randomReread(int,unsigned long, int, char *);
ioMetrics randomMixed(int,unsigned long int, int, char *);
void getDeltaUsage(const struct rusage, ioMetrics *);
ioMetrics getTotalUsage(ioMetrics *);
int getDriveInfo(const char *, driveInfo *);
