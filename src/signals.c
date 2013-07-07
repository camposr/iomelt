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
extern int fd;
extern char fileName[256];

/* handle Ctrl-C like interruptions */
void sigint_handler(int signo)
{
	if (signo == SIGINT)
	{
		if (verbose > 0)
			fprintf(stderr, "Received SIGINT, cleaning up.\n");
		if (verbose > 1)
			fprintf(stderr, "Workload file name is '%s'.\n", fileName);

		/* check if the file descriptor is open and unlinks the file */
		if (fd != -1)
		{
			close(fd);
			if (unlink(fileName) != 0)
			{
				perror("Unable to unlink workload file.");
			}
		}
		signal(SIGINT, SIG_DFL);
		kill(getpid(), SIGINT);
	}

}
