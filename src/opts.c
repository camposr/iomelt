/* iomelt - a simple disk IO benchmark

	## Copyright 2011, Rodrigo Albani de Campos (camposr@gmail.com)
	## All rights reserved.
	## 
	## This program is free software, you can redistribute it and/or
	## modify it under the terms of the "Artistic License 2.0".
	##
	## A copy of the "Artistic License 2.0" can be obtained at 
	## http://www.opensource.org/licenses/artistic-license-2.0
	##
	## This program is distributed in the hope that it will be useful,
	## but WITHOUT ANY WARRANTY; without even the implied warranty of
	## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include "iomelt.h"

extern short int verbose;
extern const char *versionNumber;


unsigned long int parseFileSize(char *optarg)
{
/* Parses the argument and converts it to a unsigned long
   User may use:
		K for KILOBYTES (1024 bytes)
		M for MEGABYTES (1024 * 1024 bytes)
		G for GIGABYTES (1024 * 1024 * 1024 bytes)

	TODO: weird things can happen in 32 bit systems, need to fix that
*/
	unsigned long int fileSize = 0;
	char *unit;

	unit = strpbrk(optarg,"KkMmGg");

	if (unit == NULL)
	{
		if (verbose > 1)
			fprintf(stderr, "parseFileSize: no unit to convert\n");
		fileSize = strtoul(optarg, NULL, 10);
	}
	else
	{
		switch(*unit)
		{
			case 'K':
			case 'k':
				myWarn(2,__FUNCTION__,"found 'K', converting to Kilobytes");
				fileSize = strtoul(optarg, NULL, 10) * KILO;
				break;
			case 'M':
			case 'm':
				myWarn(2,__FUNCTION__,"found 'M', converting to Megabytes");
				fileSize = strtoul(optarg, NULL, 10) * MEGA;
				break;
			case 'G':
			case 'g':
				myWarn(2,__FUNCTION__,"found 'G', converting to Gigabytes");
				fileSize = strtoul(optarg, NULL, 10) * GIGA;
				break;
		}
	}

	return(fileSize);
}

/* printHelp prints command line options and returns to the caller */
void printHelp(void)
{

	short int i;
	/* a structure that holds all options and option descriptions */
	typedef struct
	{
		char option;
		const char *description;
		const char *unit;
	} optionEntry;

	const optionEntry entries[] = 
	{
		{ 'b', "Block size used for IO functions (must be a power of two)", "BYTES" },
		{ 'd', "Dump data in a format that can be digested by pattern processing commands", NULL },
		{ 'D', "Print time in seconds since epoch", NULL },
		{ 'h', "Prints usage parameters", NULL },
		{ 'H', "Omit header row when dumping data", NULL },
		{ 'n', "Do NOT convert bytes to human readable format", NULL },
		{ 'o', "Do NOT display results (does not override -d)", NULL },
		{ 'O', "Reopen worload file before every test", NULL },
		{ 'p', "Directory where the test file should be created", "PATH "},
		{ 'r', "Randomize workload file name", NULL },
		{ 'R', "Try to enable Direct IO", NULL },
		{ 's', "Workload file size (default: 10Mb)", "BYTES" },
		{ 'v', "Controls the level of verbosity", NULL},
		{ 'V', "Displays version number", NULL},
		{ 0 }
	};

	printf("IOMELT Version %s\nUsage:\n",versionNumber);
	for(i = 0;entries[i].option != 0;i++)
	{
		printf("\t-%c %s\t%s\n", entries[i].option, entries[i].unit == NULL ? "\t" : entries[i].unit, entries[i].description);
	}
	printf("\n");
	printf("\t-b and -s values can be specified in bytes (default), Kilobytes (with 'K' suffix), Megabytes (with 'M'suffix), or Gigabytes (with 'G' suffix)\n");
	printf("\tUnless specified, block size value is the optimal block transfer size for the file system as returned by statvfs\n");
}

/* convert bytes to a human readable format
	based on redis bytesToHuman */
void bytesToHuman(char *s, double number)
{
	double d;
	extern bool humanize;

	/* if humanize is false user has chosen not to convert number to a human readable format */
	if (humanize == false)
	{
		sprintf(s,"%f", number);
		return;
	}

	if (number < 1024)
	{
		sprintf(s,"%luB", (long)number);
		return;
	} else if (number < (1024*1024))
	{
		d = (double)number/(1024);
		sprintf(s, "%.2fK", d);
	} else if (number < (1024L * 1024 * 1024))
	{
		d = (double)number/(1024*1024);
		sprintf(s, "%.2fM", d);
	} else if (number < (1024L * 1024 * 1024 * 1024))
	{
		d = (double)number / (1024L * 1024 * 1024);
		sprintf(s, "%.2fG", d);
	}
}

unsigned long int roundUpFileSize(const unsigned long int fileSize, const unsigned int blockSize)
{
	/* rounds up the file size to the nearest block size multiple */
	unsigned long int roundFileSize = 0;

	if (fileSize % blockSize != 0)
		roundFileSize = ((unsigned long int)fileSize/blockSize)*blockSize + blockSize;
	else
		roundFileSize = fileSize;

	myWarn(3,__FUNCTION__, "Rounded up file size, was %ld, now %ld",fileSize, roundFileSize);
	return(roundFileSize);

}

int isPowerOfTwo(unsigned int x)
{
	/* check if number is a power of two */
	return ((x != 0) && ((x & (~x+1)) == x));
}

unsigned int getBlockSize(void)
{
	/* get optimal block size as returned by statvfs() */
	char *cwd;
	struct statvfs cwdstats;
	int rc;

    #ifndef __APPLE__
	cwd = get_current_dir_name();
    #else
    cwd = getcwd(NULL, 0);
    #endif

	myWarn(3,__FUNCTION__,"Current working directory is %s",cwd);
	rc = statvfs(cwd, &cwdstats);
	free(cwd);
	if (rc == -1)
	{
		myWarn(1,__FUNCTION__,"statfs failed: %s",strerror(errno));
		return(-1);
	}
	
	return(cwdstats.f_bsize);
}

