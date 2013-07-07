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

	This project has two major objectives:

	1st - Provide consistent and reproducible IO benchmark results
	2nd - Keep code simple and well documented so anyone can easily 
		   understand what is going on under the hood

*/
	 
/* macro definitions and function prototypes are in this file */
#include "iomelt.h"

const char *versionNumber = "0.71";

/* a flag that will control the conversion of bytes in output */
/* this is enabled by default */
bool humanize = true;

/* time and usage global variables */
struct timeval initTime, lastTime, saveTime;
struct rusage	initUsage, lastUsage, saveUsage;

/* globals */
/* a global flag for verbosity */
short int verbose = 0;
/* the workload file descriptor */
int fd = -1; 
/* the file name of the workload file */
char fileName[256];

int main(int argc, char **argv)
{
	/* workload file size in bytes */
	unsigned long int fileSize = FILESIZE;
	/* write/read block size in bytes */
	int blockSize = -1;
	/* a generic variable used to hold return code in several functions */
	int rc;
	/* a generic buffer used to hold data that will be written and read */
	char *buf;
	/* a generic variable used for loop control */
	int i;
	/* a structure array that will hold usage counters */
	ioMetrics metrics[6];
	ioMetrics totalUsage;
	/* char arrays that will hold bytes in a human readable format */
	char humanBytes[256];
	char fileSizeHuman[256];
	bool randomFileName = false;
	bool directIO = false;
	bool fadvise = true;
	/* should the workload file be reopened before each test */
	bool reopen = false;
	/* dump controls the print of machine formated data at the end */
	bool dump = false;
	/* output controls if normal output should be displayed */
	bool output = true;
	/* showHeader controls if a header row should be printed when -d is used */
	bool showHeader = true;
	/* holds time of execution */
	time_t tm;
	char *humanTime;
	/* holds the host name */
	char hostName[256];
	/* a simple buffer for the stat() */
	struct stat st;



	/* set signal handler */
	if (signal(SIGINT, sigint_handler) == SIG_ERR)
		fprintf(stderr, "warn: can't catch SIGINT\n");


	/* Get initial time of execution and hostname */
	tm = time(NULL);
	humanTime = ctime(&tm);
	/* A dirty hack to chop the trailing newline that ctime() returns */
	humanTime[strlen(humanTime)-1] = '\0';
	rc = gethostname(hostName,sizeof(hostName));

	/* initialize random number generator */
	srandom((unsigned int)(time(NULL)/getpid()));

	/* parse command line options */
	while ((rc = getopt(argc, argv, "fdDvVhHnoOrRs:b:p:")) != -1)
	{
		switch(rc)
		{
			case 'v':
				verbose++;
				break;
			case 'r':
				randomFileName = true;
				break;
			case 'D':
				sprintf(humanTime, "%d", (int)tm);
				break;
			case 'f':
				fadvise = false;
				break;
			case 's':
				fileSize = parseFileSize(optarg);
				if (fileSize == 0)
				{
					myWarn(0,__FUNCTION__,"File size must be a positive integer");
					printHelp();
					exit(EXIT_FAILURE);
				}
				break;
			case 'b':
				blockSize = (int)parseFileSize(optarg);
				if (blockSize == 0)
				{
					myWarn(0,__FUNCTION__,"Block size must be a positive integer");
					printHelp();
					exit(EXIT_FAILURE);
				}
				if (!isPowerOfTwo(blockSize))
				{
					myWarn(0,__FUNCTION__,"Block size must be a power of two");
					printHelp();
					exit(EXIT_FAILURE);
				}
				break;
			case 'n':
				humanize = false;
				break;
			case 'h':
				printHelp();
				exit(EXIT_SUCCESS);
				break;
			case 'd':
				dump = true;
				break;
			case 'o':
				output = false;
				break;
			case 'O':
				reopen = true;
				break;
			case 'p':
				/* check if it is a regular path */
				rc = stat(optarg, &st);
				if (rc == 0 && S_ISDIR(st.st_mode))
				{
					/* chdir to the specified path */
					/* TODO: there's a small bug here, if the user specifies -p before -v in the command line
					he might never see the next message */
					myWarn(3,__FUNCTION__,"Switching to specified path '%s",optarg);
					rc = chdir(optarg);
					if (rc != 0)
					{
						perror("Unable to chdir()");
						exit(EXIT_FAILURE);
					}
			
				}
				else
				{
					myWarn(0,__FUNCTION__,"Invalid path: '%s'",optarg);
					exit(EXIT_FAILURE);
				}
					
				break;
			case 'R':
				myWarn(2,__FUNCTION__,"Will try to enable Direct IO");
				directIO = true;
				break;
			case 'V':
				printf("IOMELT Version %s\n",versionNumber);
				exit(EXIT_SUCCESS);
				break;
			case 'H':
				showHeader = false;
				break;
			default:
				printHelp();
				exit(EXIT_FAILURE);
		}
	}

	/* 	if the user didn't specify a block size iomelt will try to figure out the
		optimal block size for the file system by using statfs */
	if (blockSize == -1)
	{
		blockSize = getBlockSize();
		if (blockSize == -1)
		{
			myWarn(1,__FUNCTION__,"Unable to get optimal block size, using %s",BLOCKSIZE);
			blockSize = BLOCKSIZE;
		}
		else
			myWarn(2,__FUNCTION__,"Block size is %d",blockSize);
	}

	/* Convert file size to a human readable format */
	bytesToHuman(fileSizeHuman, fileSize);

	/* sets workload file name */
	if (randomFileName == true)
	{
		//srandom((unsigned int)(tm + getpid()));
		snprintf(fileName, sizeof(fileName), "%s-%ld", IOFILE, random()<<getpid());

	}
	else
	{
		snprintf(fileName, sizeof(fileName), "%s", IOFILE);
	}
	myWarn(3,__FUNCTION__, "Workload file name is '%s'", fileName);

	/* make sure that file size is larger than block size */
	if (fileSize <= blockSize)
	{
		myWarn(0,__FUNCTION__,"File size has to be greater than block size\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		/* 	Round up file size to the nearest blocksize multiple */
		fileSize = roundUpFileSize(fileSize, blockSize);
	}

	/* Open workload file */

	fd = myOpen(fileName, directIO);
	if (fd == -1)
	{
		myWarn(0,__FUNCTION__, "Unable to open workload file: %s",strerror(errno));
		exit(EXIT_FAILURE);
	}
	else
	{
		myWarn(2,__FUNCTION__,"Successfully opened workload file '%s'", fileName);
	}

	/* allocate buffer memory and fill buffer with zeroes */
	/* buffer is aligned so it works with direct i/o */
	rc = posix_memalign((void**)&buf,sysconf(_SC_PAGESIZE),blockSize);
	if (rc != 0)
	{
		myWarn(0,__FUNCTION__,
			"Unable to allocate memory for the buffer: %s",
			rc == EINVAL ? "Wrong alignment argument" : "Insufficient memory to fulfill the allocation request");
		exit(EXIT_FAILURE);
	}

	if (buf == NULL)
	{
		myWarn(0,__FUNCTION__,"Unable to allocate memory for the buffer");
		exit(EXIT_FAILURE);
	}

	(void)memset(buf, 0, blockSize);

	if (output == true)
		printf("Serial write test - File size: %s - Block size: %d\n", fileSizeHuman, blockSize);
	/************************/
	/*     Serial Write     */
	/************************/
	metrics[0] = serialWrite(fd,fileSize,blockSize,buf);

	if (output == true) 
	{
		printf("Serial write test finished\n");
		printf("write() calls: %ld\n",fileSize/blockSize);
		printf("Wallclock seconds: %f\n",metrics[0].wallClockTime);
		printf("write() calls per second: %f\n", metrics[0].totalCalls / metrics[0].wallClockTime);
		bytesToHuman(humanBytes, (fileSize / metrics[0].wallClockTime));
		printf("write() bytes per second: %s\n", humanBytes );
		if (verbose > 0)
		{
			printf("Blocks in/out: %ld/%ld\n",metrics[0].blocksIn, metrics[0].blocksOut);
		}
	}
	/* close and reopen the file descriptor */
	if (reopen == true)
		fd = myOpen(fileName, directIO);
	
	rc = lseek(fd,0,SEEK_SET);

	if (rc < 0)
	{
		myWarn(0,__FUNCTION__,"Unable to lseek: %s",strerror(errno));
		exit(EXIT_FAILURE);
	}



	/* *TRY* to minimize buffer cache effect */
	/* There's no guarantee that the file will be removed from buffer cache though */
	/* Keep in mind that buffering will happen at some level */
	#ifndef __APPLE__
	if (fadvise == true)
	{
		rc = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
		if (rc !=0)
		{
			myWarn(1,__FUNCTION__,"Unable to use posix_fadvise: %s",strerror(errno));
		}
	}
	#else
	rc = fcntl(fd, F_NOCACHE, 0);
	if (rc == -1) perror("main() - [__APPLE__] fcntl F_NOCACHE error");
	rc = fcntl(fd, F_RDAHEAD, 0);
	if (rc == -1) perror("main() - [__APPLE__] fcntl F_RDAHEAD error");
	#endif


	
	if (output == true)
		printf("\nSerial read test - File size: %s - Block size: %d\n", fileSizeHuman, blockSize);
		
	/************************/
	/*    Serial Read       */
	/************************/
	metrics[1] = serialRead(fd, fileSize, blockSize, buf);

	if (output == true)
	{
	
		printf("Serial read test finished\n");
		printf("read() calls: %d\n",metrics[1].totalCalls);
		printf("Wallclock seconds: %f\n",metrics[1].wallClockTime);
		printf("read() calls per second: %f\n", metrics[1].totalCalls / metrics[1].wallClockTime);
		bytesToHuman(humanBytes, (fileSize / metrics[1].wallClockTime));
		printf("read() bytes per second: %s\n", humanBytes);
		if (verbose > 0)
		{
			printf("Blocks in/out: %ld/%ld\n",metrics[1].blocksIn, metrics[1].blocksOut);
		}
	}


	if (reopen == true)
			fd = myOpen(fileName, directIO);
	if (output == true)
		printf("\nRandom rewrite test - File size: %s - Block size: %d\n", fileSizeHuman, blockSize);
	memset(buf,1,blockSize);

	/************************/
	/*   Random Rewrite     */
	/************************/
	metrics[2] = randomRewrite(fd, fileSize, blockSize, buf);

	if (output == true)
	{
		printf("Random rewrite test finished\n");
		printf("write() calls: %d\n",metrics[2].totalCalls);
		printf("Wallclock seconds: %f\n",metrics[2].wallClockTime);
		printf("write() calls per second: %f\n", metrics[2].totalCalls / metrics[2].wallClockTime);
		bytesToHuman(humanBytes, (fileSize / metrics[2].wallClockTime));
		printf("write() bytes per second: %s\n", humanBytes );
		if (verbose > 0)
		{
			printf("Blocks in/out: %ld/%ld\n",metrics[2].blocksIn, metrics[2].blocksOut);
		}
	}


	/* Random reread test */
	if (reopen == true)
		fd = myOpen(fileName, directIO);
	if (output == true)
		printf("\nRandom reread test - File size: %s - Block size: %d\n", fileSizeHuman, blockSize);
	#ifndef __APPLE__
	if (fadvise == true)
	{
		rc = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
		if (rc!=0)
			perror("main() - Unable to use posix_fadvise() on workload file");
	}
	#else
   rc = fcntl(fd, F_NOCACHE, 0);
   if (rc == -1) perror("main() - [__APPLE__] fcntl F_NOCACHE error");
   rc = fcntl(fd, F_RDAHEAD, 0);
   if (rc == -1) perror("main() - [__APPLE__] fcntl F_RDAHEAD error");
    #endif

	/************************/
	/*   Random Reread     */
	/************************/

	metrics[3] = randomReread(fd,fileSize,blockSize,buf);

	if (output == true)
	{
		printf("Random read test finished\n");
		printf("read() calls: %d\n",metrics[3].totalCalls);
		printf("Wallclock seconds: %f\n",metrics[3].wallClockTime);
		printf("read() calls per second: %f\n", metrics[3].totalCalls / metrics[3].wallClockTime);
		bytesToHuman(humanBytes, (fileSize / metrics[3].wallClockTime));
		printf("read() bytes per second: %s\n", humanBytes);
		if (verbose > 0)
		{
			printf("Blocks in/out: %ld/%ld\n",metrics[3].blocksIn, metrics[3].blocksOut);
		}
	}

	/* Random mixed read/write test*/
	if (reopen == true)
		fd = myOpen(fileName, directIO);
	if (output == true)
		printf("\nRandom mixed read/write test - File size: %s - Block size: %d\n", fileSizeHuman, blockSize);
	//srandom((unsigned int)(tm + getpid()));
	#ifndef __APPLE__
	if (fadvise == true)
	{
		rc = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
		if (rc!=0)
			perror("main() - Unable to use posix_fadvise() on workload file");
	}
	#else
   rc = fcntl(fd, F_NOCACHE, 0);
   if (rc == -1) perror("main() - [__APPLE__] fcntl F_NOCACHE error");
   rc = fcntl(fd, F_RDAHEAD, 0);
   if (rc == -1) perror("main() - [__APPLE__] fcntl F_RDAHEAD error");
    #endif

	memset(buf,2,blockSize);

	/************************/
	/*   Random Mixed       */
	/************************/
	metrics[4] = randomMixed(fd, fileSize, blockSize, buf);
	if (output == true)
	{
		printf("Random read/write test finished\n");
		printf("read()/write() calls: %d\n",metrics[4].totalCalls);
		printf("Wallclock seconds: %f\n",metrics[4].wallClockTime);
		printf("read()/write() calls per second: %f\n", metrics[4].totalCalls/ metrics[4].wallClockTime);
		bytesToHuman(humanBytes, (fileSize / metrics[4].wallClockTime));
		printf("read()/write() bytes per second: %s\n", humanBytes);
		if (verbose > 0)
		{
			printf("Blocks in/out: %ld/%ld\n",metrics[4].blocksIn, metrics[4].blocksOut);
		}
	}
	
		


	/* Dump detailed metrics of program execution */

	totalUsage = getTotalUsage(metrics);

	if (output == true)
	{
		printf("\n\nFinished all tests\n");
		printf("Total wallclock time: %f\n", totalUsage.wallClockTime);
		printf("Blocks in/out: %ld/%ld\n", totalUsage.blocksIn, totalUsage.blocksOut);
		printf("CPU user/system time: %f/%f\n", totalUsage.userTime, totalUsage.systemTime);
	}


//	dumpTotalUsage(metrics);

	/* Close the workload file descriptor */
	rc = close(fd);
	if (rc != 0)
	{
		perror("Unable to close workload file");
		exit(EXIT_FAILURE);
	}

	/* Dump data in a format that can be digested by R and awk */
	if (dump == true)
	{
		if (showHeader == true)
			printf("#Date;Hostname;Test;File Size;Block Size;Total Time;Calls per second;Bytes per second\n");

		for (i=0;i<5;i++)
		{
			printf("%s;%s;%s;%lu;%d;%f;%f;%f\n", 
				humanTime, hostName, metrics[i].testName, 
				fileSize, blockSize, metrics[i].wallClockTime,
				(fileSize / blockSize) / metrics[i].wallClockTime,
				fileSize / metrics[i].wallClockTime );
		}
	}



	/* clean up before we leave */
	free(buf);
	rc = unlink(fileName);
	if (rc != 0)
	{
		perror("Unable to unlink workload file");
		exit(EXIT_FAILURE);
	}

return(0);
}


