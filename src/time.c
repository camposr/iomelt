#include "iomelt.h"

double getTime(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		fprintf(stderr, "%s: Error while getting time: %s\n", __func__, strerror(errno));

	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double getDelta(double lastTime)
{
	return getTime() - lastTime;
}

void getDeltaUsage(const struct rusage initUsage, ioMetrics *metrics)
{
	struct rusage endUsage;
	int rc;

	rc = getrusage(RUSAGE_SELF, &endUsage);
	if (rc == -1)
	{
		myWarn(0, __func__, "Unable to call getrusage(): %s", strerror(errno));
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
	double weightedLatSum = 0.0;

	memset(&sum, 0, sizeof(ioMetrics));
	sum.minLatency = DBL_MAX;

	for(i = 0; i < NUM_TESTS; i++)
	{
		sum.blocksIn      += metrics[i].blocksIn;
		sum.blocksOut     += metrics[i].blocksOut;
		sum.userTime      += metrics[i].userTime;
		sum.systemTime    += metrics[i].systemTime;
		sum.wallClockTime += metrics[i].wallClockTime;
		sum.totalCalls    += metrics[i].totalCalls;

		if (metrics[i].minLatency < sum.minLatency)
			sum.minLatency = metrics[i].minLatency;
		if (metrics[i].maxLatency > sum.maxLatency)
			sum.maxLatency = metrics[i].maxLatency;

		/* weighted average: each test contributes avg * its call count */
		weightedLatSum += metrics[i].avgLatency * metrics[i].totalCalls;
	}

	sum.avgLatency = (sum.totalCalls > 0) ?
	                 weightedLatSum / sum.totalCalls : 0.0;

	return(sum);
}
