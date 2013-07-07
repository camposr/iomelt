#include "iomelt.h"

double getTime(void)
{
	struct timeval currentTime;
	double returnTime;
	int rc = 0;

	rc = gettimeofday(&currentTime, NULL);

	if (rc != 0)
		fprintf(stderr, "%s: Error while getting time: %s\n", __FUNCTION__, strerror(errno));
	
	returnTime = (double)currentTime.tv_sec + (double)currentTime.tv_usec/1000000;

	return(returnTime);
}

double getDelta(double lastTime)
{
	struct timeval currentTime;
	double delta;
	int rc = 0;

	rc = gettimeofday(&currentTime, NULL);

	if (rc != 0)
		fprintf(stderr, "%s: Error while getting time: %s\n", __FUNCTION__, strerror(errno));
	
	delta = ((double)currentTime.tv_sec + (double)currentTime.tv_usec/1000000) -
	        lastTime;

	return(delta);
}

void getDeltaUsage(const struct rusage initUsage, ioMetrics *metrics)
{
	struct rusage endUsage;
	int rc;

	rc = getrusage(RUSAGE_SELF, &endUsage);
	if (rc == -1)
	{
		myWarn(0,__FUNCTION__,"Unable to call getrusage(): %s",strerror(errno));
	}

	metrics->blocksIn = 
		endUsage.ru_inblock - initUsage.ru_inblock;
	metrics->blocksOut = 
		endUsage.ru_oublock - initUsage.ru_oublock;

	metrics->userTime =
		((double)endUsage.ru_utime.tv_sec + (double)endUsage.ru_utime.tv_usec / 1000000) - 
		((double)initUsage.ru_utime.tv_sec + (double)initUsage.ru_utime.tv_usec / 1000000);

	metrics->systemTime =
		((double)endUsage.ru_stime.tv_sec + (double)endUsage.ru_stime.tv_usec / 1000000) - 
		((double)initUsage.ru_stime.tv_sec + (double)initUsage.ru_stime.tv_usec / 1000000);


}

ioMetrics getTotalUsage(ioMetrics *metrics)
{
	ioMetrics sum;
	int i;

	memset(&sum,0,sizeof(ioMetrics));

	for(i=0;i<5;i++)
	{
		//fprintf(stderr, "%s\n",metrics[i].testName);
		sum.blocksIn 		+= metrics[i].blocksIn;
		sum.blocksOut 		+= metrics[i].blocksOut;
		sum.userTime 		+= metrics[i].userTime;
		sum.systemTime 		+= metrics[i].systemTime;
		sum.wallClockTime 	+= metrics[i].wallClockTime;
		sum.totalCalls 		+= metrics[i].totalCalls;
	}
	return(sum);
}