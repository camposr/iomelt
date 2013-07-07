/* this file contains the methods used to output warning and error messages */

#include "iomelt.h"

void myWarn (short int level, const char *caller, const char *fmt, ...)
{
	va_list ap;
	char *msg;
	int rc;
	extern short int verbose;

	/* level should be defined as follows:
		0 - critical error, mostly fatal
		1 - warning
		2 - information
		3 - debugging
	*/

	if (level <= verbose)
	{
		va_start(ap, fmt);
		rc = vasprintf(&msg, fmt, ap);
		if (rc <=0)
			fprintf(stderr, "%s: Unable to allocate memory for error message\n",__FUNCTION__);
		else
			fprintf(stderr, "%s: %s\n", caller, msg);
		
		va_end(ap);	
		/* man page says that contents of msg is undefined in case of error so I'll call free() no matter what */
		free(msg);
	}
}