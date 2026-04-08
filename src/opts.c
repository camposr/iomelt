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
*/
	unsigned long long value;
	char *unit;

	unit = strpbrk(optarg, "KkMmGg");

	/* parse the numeric part using strtoull to avoid 32-bit overflow */
	value = strtoull(optarg, NULL, 10);

	if (unit != NULL)
	{
		switch(*unit)
		{
			case 'K':
			case 'k':
				myWarn(2, __func__, "found 'K', converting to Kilobytes");
				value *= KILO;
				break;
			case 'M':
			case 'm':
				myWarn(2, __func__, "found 'M', converting to Megabytes");
				value *= MEGA;
				break;
			case 'G':
			case 'g':
				myWarn(2, __func__, "found 'G', converting to Gigabytes");
				value *= GIGA;
				break;
		}
	}
	else
	{
		if (verbose > 1)
			fprintf(stderr, "parseFileSize: no unit to convert\n");
	}

	return (unsigned long int)value;
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
		{ 'b', "Block size for IO operations (must be a power of two)", "BYTES" },
		{ 'd', "Dump results as semicolon-separated CSV to stdout", NULL },
		{ 'D', "Print timestamp as seconds since epoch instead of a date string", NULL },
		{ 'f', "Disable posix_fadvise() cache hints (Linux only)", NULL },
		{ 'h', "Print usage information and exit", NULL },
		{ 'H', "Omit the header row when dumping CSV data", NULL },
		{ 'k', "Keep the workload file on exit instead of removing it", NULL },
		{ 'n', "Do not convert byte counts to human-readable format", NULL },
		{ 'o', "Do not display results (does not suppress -d output)", NULL },
		{ 'O', "Reopen the workload file before every test", NULL },
		{ 'p', "Directory where the workload file should be created", "PATH" },
		{ 'r', "Append a random suffix to the workload file name", NULL },
		{ 'R', "Disable Direct IO (Direct IO is enabled by default)", NULL },
		{ 's', "Workload file size (default: 10MB)", "BYTES" },
		{ 'v', "Increase verbosity (repeat for more detail: -vvv)", NULL },
		{ 'V', "Print version number and exit", NULL },
		{ 'w', "Append CSV output to FILE instead of stdout (implies -d)", "FILE" },
		{ 'x', "Dry run: display parameters and exit without running tests", NULL },
		{ 0 }
	};

	printf("IOMELT Version %s\n", versionNumber);
	printf("Usage: iomelt [OPTIONS] [-b BYTES] [-s BYTES] [-p PATH] [-w FILE]\n\n");
	for (i = 0; entries[i].option != 0; i++)
	{
		printf("  -%c %-6s  %s\n",
			entries[i].option,
			entries[i].unit ? entries[i].unit : "",
			entries[i].description);
	}
	printf("\n");
	printf("  -b and -s accept a unit suffix: K (kilobytes), M (megabytes), G (gigabytes).\n");
	printf("  If -b is not specified, the optimal block size is obtained from statvfs(3).\n");
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
		snprintf(s, 256, "%f", number);
		return;
	}

	if (number < 1024)
	{
		snprintf(s, 256, "%luB", (long)number);
		return;
	} else if (number < (1024*1024))
	{
		d = (double)number/(1024);
		snprintf(s, 256, "%.2fK", d);
	} else if (number < (1024L * 1024 * 1024))
	{
		d = (double)number/(1024*1024);
		snprintf(s, 256, "%.2fM", d);
	} else if (number < (1024L * 1024 * 1024 * 1024))
	{
		d = (double)number / (1024L * 1024 * 1024);
		snprintf(s, 256, "%.2fG", d);
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

	myWarn(3, __func__, "Rounded up file size, was %ld, now %ld", fileSize, roundFileSize);
	return(roundFileSize);

}

int isPowerOfTwo(int x)
{
	/* check if number is a power of two */
	myWarn(3, __func__, "Checking if block size is a power of two: %d", x);
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

	myWarn(3, __func__, "Current working directory is %s", cwd);
	rc = statvfs(cwd, &cwdstats);
	free(cwd);
	if (rc == -1)
	{
		myWarn(1, __func__, "statfs failed: %s", strerror(errno));
		return(-1);
	}

	return(cwdstats.f_bsize);
}
