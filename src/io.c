#include "iomelt.h"

int myOpen(const char *fileName, const bool directIO)
{
	/* opens or reopens a file and returns its file descriptor to the caller */
	static int fd = -1;
	int flags = 0;
	int rc = 0;

	if (fd != -1)
	{
		myWarn(3, __func__, "File descriptor already exists, closing it");

		rc = close(fd);
		if (rc != 0)
		{
			myWarn(1, __func__, "Unable to close file descriptor: %s", strerror(errno));
		}
		flags = O_RDWR | O_CREAT | O_SYNC;

	}
	else
		flags = O_RDWR | O_CREAT | O_TRUNC | O_SYNC;

	if (directIO == true)
	{
		myWarn(3, __func__, "Setting the flag for Direct IO");
#ifdef O_DIRECT
		flags |= O_DIRECT;
#endif
	}

	fd = open(fileName, flags, 0644);

	if (fd == -1)
	{
		myWarn(0, __func__, "Unable to open workload file (try disabling Direct IO): %s", strerror(errno));
	}
	else
	{
		myWarn(3, __func__, "Successfully opened workload file");
#ifdef __APPLE__
		if (directIO == true)
		{
			rc = fcntl(fd, F_NOCACHE, 1);
			if (rc == -1) perror("myOpen() - [__APPLE__] fcntl F_NOCACHE error");
		}
#endif
	}

	return fd;
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
	double t, opStart, opLatency;
	double minLat = DBL_MAX, maxLat = 0.0, sumLat = 0.0;
	struct rusage initUsage;
	ioMetrics metrics;

	/* serial write test */
	/* this test actually fills up the workload file that will be used for the remaining tests */
	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
	}

	for(i=0; i < (int)(fileSize / blockSize); i++)
	{
		opStart = getTime();
		rc = write(fd, buf, blockSize);
		opLatency = getDelta(opStart);

		if (rc != blockSize)
		{
			myWarn(0, __func__, "Unable to write: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (opLatency < minLat) minLat = opLatency;
		if (opLatency > maxLat) maxLat = opLatency;
		sumLat += opLatency;
	}
	rc = fsync(fd);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to fsync(): %s", strerror(errno));
	}

	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	metrics.minLatency = minLat;
	metrics.maxLatency = maxLat;
	metrics.avgLatency = (i > 0) ? sumLat / i : 0.0;
	snprintf(metrics.testName, sizeof(metrics.testName), "Serial Write");

	return(metrics);
}

ioMetrics serialRead(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t, opStart, opLatency;
	double minLat = DBL_MAX, maxLat = 0.0, sumLat = 0.0;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
	}

	opStart = getTime();
	while ((rc = read(fd, buf, blockSize)) != 0)
	{
		opLatency = getDelta(opStart);

		if (rc != blockSize)
		{
			myWarn(0, __func__, "Unable to read: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (opLatency < minLat) minLat = opLatency;
		if (opLatency > maxLat) maxLat = opLatency;
		sumLat += opLatency;
		i++;

		opStart = getTime();
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	metrics.minLatency = minLat;
	metrics.maxLatency = maxLat;
	metrics.avgLatency = (i > 0) ? sumLat / i : 0.0;
	snprintf(metrics.testName, sizeof(metrics.testName), "Serial Read");

	return(metrics);
}

ioMetrics randomRewrite(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t, opStart, opLatency;
	double minLat = DBL_MAX, maxLat = 0.0, sumLat = 0.0;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
	}

	for(i=0; i < (int)(fileSize / blockSize); i++)
	{
		rc = myRandomSeek(fd, fileSize, blockSize);
		if (rc == -1)
		{
			myWarn(0, __func__, "lseek failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		opStart = getTime();
		rc = write(fd, buf, blockSize);
		opLatency = getDelta(opStart);

		if (rc != blockSize)
		{
			myWarn(0, __func__, "write() returned %d", rc);
			perror("Random Rewrite - Unable to write");
			exit(EXIT_FAILURE);
		}

		if (opLatency < minLat) minLat = opLatency;
		if (opLatency > maxLat) maxLat = opLatency;
		sumLat += opLatency;
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	metrics.minLatency = minLat;
	metrics.maxLatency = maxLat;
	metrics.avgLatency = (i > 0) ? sumLat / i : 0.0;
	snprintf(metrics.testName, sizeof(metrics.testName), "Random Rewrite");

	return(metrics);
}

ioMetrics randomReread(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t, opStart, opLatency;
	double minLat = DBL_MAX, maxLat = 0.0, sumLat = 0.0;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
	}

	for(i=0; i < (int)(fileSize / blockSize); i++)
	{
		rc = myRandomSeek(fd, fileSize, blockSize);
		if (rc == -1)
		{
			myWarn(0, __func__, "lseek failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		opStart = getTime();
		rc = read(fd, buf, blockSize);
		opLatency = getDelta(opStart);

		if (rc != blockSize)
		{
			myWarn(0, __func__, "Unable to read: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (opLatency < minLat) minLat = opLatency;
		if (opLatency > maxLat) maxLat = opLatency;
		sumLat += opLatency;
	}
	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	metrics.minLatency = minLat;
	metrics.maxLatency = maxLat;
	metrics.avgLatency = (i > 0) ? sumLat / i : 0.0;
	snprintf(metrics.testName, sizeof(metrics.testName), "Random Reread");

	return(metrics);

}

ioMetrics randomMixed(int fd, unsigned long int fileSize, int blockSize, char *buf)
{
	int i = 0;
	int rc = 0;
	double t, opStart, opLatency;
	double minLat = DBL_MAX, maxLat = 0.0, sumLat = 0.0;
	struct rusage initUsage;
	ioMetrics metrics;

	t = getTime();
	rc = getrusage(RUSAGE_SELF, &initUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
	}

	for (i=0; i < (int)(fileSize / blockSize); i++)
	{
		rc = myRandomSeek(fd, fileSize, blockSize);
		if (rc == -1)
		{
			myWarn(0, __func__, "lseek failed: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		opStart = getTime();
		if (((int)random() % 2) == 0)
		{
			/* read */
			rc = read(fd, buf, blockSize);
			opLatency = getDelta(opStart);
			if (rc != blockSize)
			{
				myWarn(0, __func__, "Unable to read: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			/* write */
			rc = write(fd, buf, blockSize);
			opLatency = getDelta(opStart);
			if (rc != blockSize)
			{
				myWarn(0, __func__, "Unable to write: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}

		if (opLatency < minLat) minLat = opLatency;
		if (opLatency > maxLat) maxLat = opLatency;
		sumLat += opLatency;
	}

	metrics.wallClockTime = getDelta(t);
	getDeltaUsage(initUsage, &metrics);

	metrics.totalCalls = i;
	metrics.minLatency = minLat;
	metrics.maxLatency = maxLat;
	metrics.avgLatency = (i > 0) ? sumLat / i : 0.0;
	snprintf(metrics.testName, sizeof(metrics.testName), "Random Mixed");

	return(metrics);

}
