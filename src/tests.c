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
		flags = flags| O_DIRECT;
	}

	/* try to open using O_DIRECT */
	
	fd = open(fileName, flags, 0644);


	if (fd == -1)
		myWarn(0,__FUNCTION__,"Unable to open workload file (try disabling Direct IO): %s",strerror(errno));
	else
		myWarn(3,__FUNCTION__,"Successfully opened workload file");
	
	return(fd);
}

off_t myRandomSeek(int fd, unsigned long int fileSize, int blockSize)
{
	unsigned long int pos;
	int rc;

	/* get a properly aligned position within the file */
	pos = (random() % ((fileSize - blockSize)/blockSize))*blockSize;

	rc = lseek(fd, pos, SEEK_SET);

	//fprintf(stderr,"pos = %ld\n",pos);

	return(rc);

}