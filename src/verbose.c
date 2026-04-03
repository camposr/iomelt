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
		va_end(ap);
		if (rc < 0)
			fprintf(stderr, "%s: Unable to format error message\n", __func__);
		else
		{
			fprintf(stderr, "%s: %s\n", caller, msg);
			free(msg);
		}
	}
}
