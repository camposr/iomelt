IOMELT - Rodrigo Albani de Campos (camposr<at>gmail<dot>com)

* What is iomelt?
IOMELT is a benchmark tool for storage devices.
It is designed to be simple to use and understand.
Currently it runs only on Linux and Mac OS X.

* How do I build IOMELT?
It should be as simple as:

	% make

To use the autoconf/automake build system instead:

	% ./autogen.sh
	% ./configure
	% make

* Why build yet another benchmark tool?
I needed something simpler, easier to understand and - more importantly - easier to explain
than currently available tools.

* What are the main goals of this project?
	1. Provide consistent and reproducible results
	2. Keep code simple and well documented so that anyone can understand how it works

* How does iomelt work?
It runs a series of five sequential tests:

	1. Serial Write test
	2. Serial Read test
	3. Random Rewrite test
	4. Random Reread test
	5. Random Mixed Read/Write test

* What are the current options?
As of April 2026, version 1.2 supports the following command line options:

	-b BYTES	Block size used for IO functions (must be a power of two)
	-d		Dump data in a format that can be digested by pattern processing commands
	-D		Print time in seconds since epoch
	-f		Disable posix_fadvise() cache hints (Linux only)
	-h		Prints usage parameters
	-H		Omit header row when dumping data
	-n		Do NOT convert bytes to human readable format
	-o		Do NOT display results (does not override -d)
	-O		Reopen workload file before every test
	-p PATH		Directory where the test file should be created
	-r		Randomize workload file name
	-R		Try to enable Direct IO
	-s BYTES	Workload file size (default: 10MB)
	-v		Controls the level of verbosity
	-V		Displays version number

Block size and file size (-b and -s options) are in bytes (default), Kilobytes ('K' suffix),
Megabytes ('M' suffix), or Gigabytes ('G' suffix).
Unless specified, block size value is the optimal block transfer size for the file system
as returned by statvfs().


* How can I use it?
Typically you can use it on the command line and it will output its results in a human
readable format.
You can configure it as a cron job and use the '-d' option to create time series charts or
scatter plots with the results.

* Why does it only work on Linux and Mac OS X?
Mostly because the system calls that try to disable buffer caching are not portable.

* I have an awesome idea and would like to help!
Send me an e-mail at camposr<at>gmail<dot>com

------- VERSION HISTORY ---------

- 0.4
	Added -p option to specify the path
	Added the mixed read/write test
	Simplified the random seed generator
	Code cleanup
	Check for return codes for read and write operations

- 0.5
	Added the parseFileSize function that parses the '-s' and '-b' command line options
	to allow the unit suffixes
	Makefile now has an 'install' target

- 0.5.1
	Added a man page
	Added a version number to the printHelp() output
	Units may be specified in lower case characters
	OS X support (port by Gleicon Moraes, thanks!)

- 0.6 (August 2012)
	Data dump now includes file and block size
	Introduced the -H command line option

- 0.7 (November 2012)
	The block size has no fixed value anymore; instead it uses statvfs() to retrieve
	the optimal value for the underlying file system
	IOMelt now supports Direct IO (-R option)
	In order to support Direct IO (O_DIRECT) the buffer is created using
	posix_memalign() instead of malloc()
	There is a new option (-O) that will cause IOMelt to reopen the workload file
	before each test
	All test source code was moved to a file called io.c
	Diagnostic, error and debugging information are now printed using the myWarn()
	function defined in verbose.c
	getUsage(), startUsage() and resetUsage() are no longer used and were replaced
	with a much cleaner interface
	All lseek() calls are now wrapped in a function called myRandomSeek(); the seek
	position is always a multiple of the block size

- 1.0 (April 2026)
	Fixed incorrect file descriptor sentinel (0 instead of -1) in myOpen(), which
	could cause stdin to be closed silently on some systems
	Fixed F_NOCACHE being applied unconditionally on macOS regardless of whether
	Direct IO (-R) was requested
	Fixed O_DIRECT portability: detection now uses #ifdef O_DIRECT instead of
	#ifndef __APPLE__, which correctly handles other BSDs
	Fixed dead unreachable code in randomMixed()
	Fixed serialWrite() silently continuing after a write failure; it now aborts
	consistently with the other tests
	Fixed vasprintf() error check in myWarn() (rc <= 0 incorrectly rejected
	successful empty-string allocations; corrected to rc < 0)
	Fixed free() being called on an undefined pointer when vasprintf() fails
	Fixed integer overflow in parseFileSize() when using the 'G' suffix on 32-bit
	systems by using strtoull() for intermediate arithmetic
	Replaced gettimeofday() with clock_gettime(CLOCK_MONOTONIC) for monotonic
	timing that is immune to NTP and system clock adjustments
	Replaced hardcoded magic number 5 with NUM_TESTS constant throughout
	All compound macro definitions are now correctly parenthesized (FILESIZE,
	BLOCKSIZE, MEGA, GIGA)
	Replaced non-standard __FUNCTION__ with C99 standard __func__ throughout
	Replaced sprintf() with snprintf() throughout
	Makefile: removed interactive rm -I flag; _GNU_SOURCE is now conditionally
	set on Linux only; hardcoded /bin/tar path replaced with tar
	Added autoconf/automake build system support (configure.ac, Makefile.am,
	autogen.sh)

- 1.1 (April 2026)
	Added a fsync() call after the write operations loop

- 1.2 (April 2026)
	Added per-operation IO latency measurement (min/avg/max) to all benchmark tests
	Added drive information detection (device, model, vendor, type, filesystem)
	Fixed unused variable warning for posix_fadvise hint flag
