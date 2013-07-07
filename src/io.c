#include "iomelt.h"

int myOpen(const char *fileName, const bool directIO)
{
	/* opens or reopens a file and returns its file descriptor to the caller */
	static int fd = 0;
	int flags = 0;
	int rc = 0;

	if (fd != 0)
	{
		myWarn(3,__FUNCTION__,"File descriptor already exists, closing it");

		rc = close(fd);
		if (rc != 0)
		{
			myWarn(1,__FUNCTION__,"Unable to close file descriptor: %s",strerror(errno));
		}
		flags = O_RDWR | O_CREAT | O_SYNC;

	}
	else
		flags = O_RDWR | O_CREAT | O_TRUNC | O_SYNC;

	

	if (directIO == true)
	{
		myWarn(3,__FUNCTION__, "Will try to enable Direct IO");
        #ifndef __APPLE__
		flags = flags| O_DIRECT;
        #endif
	}

	/* try to open using O_DIRECT */
	
	fd = open(fileName, flags, 0644);


	if (fd == -1)
		myWarn(0,__FUNCTION__,"Unable to open workload file (try disabling Direct IO): %s",strerror(errno));
	else
		myWarn(3,__FUNCTION__,"Successfully opened workload file");
    #ifdef __APPLE__
        rc = fcntl(fd, F_NOCACHE, 1);
        if (rc == -1) perror("main() - [__APPLE__] fcntl F_NOCACHE error");
    #endif
	return(fd);
}

off_t myRandomSeek(int fd, unsigned long int fileSize, int blockSize)
{
	unsigned long int pos;
	int rc;

	/* get a properly aligned position within the file */
	pos = (random() % ((fileSize - blockSize)/blockSize))*blockSize;

	rc = lseek(fd, pos, SEEK_SET);


	return(rc);

}

ioMetrics serialWrite(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t;
	struct rusage initUsage;
	ioMetrics metrics;

	/* serial write test */
	/* this test actually fills up the workload file that will be used for the remaining tests */
	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0,__FUNCTION__,"Unable to call getrusage(): %s",strerror(errno));
	}

	for(i=0; i < fileSize / blockSize; i++)
	{
		rc = write(fd,buf,blockSize);
		if (rc != blockSize)
		{
			myWarn(0,__FUNCTION__,"Unable to write: %s",strerror(errno));
		}
		
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);


	metrics.totalCalls = i;
	sprintf(metrics.testName, "Serial Write");

	return(metrics);
}

ioMetrics serialRead(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);

	while ((rc = read(fd, buf, blockSize)) != 0)
	{
		if (rc != blockSize){
			myWarn(0,__FUNCTION__,"Unable to read: %s",strerror(errno));
		}
		else
		{
			i++;			
		}

	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	sprintf(metrics.testName, "Serial Read");

	return(metrics);		
}

ioMetrics randomRewrite(int fd,unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t;
	struct rusage initUsage;	
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);

	for(i=0; i < fileSize / blockSize; i++)
	{
			rc = myRandomSeek(fd,fileSize,blockSize);
			if (rc == -1)
			{
				myWarn(0,__FUNCTION__,"lseek failed: %s",strerror(errno));
				exit(EXIT_FAILURE);
			}
				
			rc = write(fd,buf,blockSize);
			if (rc != blockSize)
			{
				myWarn(3,__FUNCTION__, "write() returned %d\n",rc);
				perror("Random Rewrite - Unable to write");
				exit(EXIT_FAILURE);
			}
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	sprintf(metrics.testName, "Random Rewrite");

	return(metrics);
}

ioMetrics randomReread(int fd,unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);

	for(i=0; i < fileSize / blockSize; i++)
	{
		rc = myRandomSeek(fd,fileSize,blockSize);
		if (rc == -1)
		{
			myWarn(0,__FUNCTION__,"lseek failed: %s",strerror(errno));
			exit(EXIT_FAILURE);
		}

		rc = read(fd,buf,blockSize);
		if (rc != blockSize)
		{
			perror("Random Reread - Unable to read");
			exit(EXIT_FAILURE);
		}
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	sprintf(metrics.testName, "Random Reread");

	return(metrics);

}

ioMetrics randomMixed(int fd,unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t;
	struct rusage initUsage;
	ioMetrics metrics;


	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);

	for (i=0; i < fileSize / blockSize; i++)
	{
		rc = myRandomSeek(fd,fileSize,blockSize);
		if (rc == -1)
		{
			myWarn(0,__FUNCTION__,"lseek failed: %s",strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (rc == -1)
			perror("failed during lseek");
		if (((int)random() % 2) == 0)
		{
			/* read */
			rc = read(fd,buf,blockSize);
			if (rc != blockSize)
			{
				perror("Random Mixed Read/Write - Unable to read");
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			/* write */
			rc = write(fd,buf,blockSize);
			if (rc != blockSize)
			{
				perror("Random Mixed Read/Write - Unable to write");
				exit(EXIT_FAILURE);
			}
		}
	}

	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	sprintf(metrics.testName, "Random Mixed");

	return(metrics);

}
