AC_PREREQ([2.69])
AC_INIT([iomelt], [1.2], [camposr@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC

# Enable GNU extensions (_GNU_SOURCE on Linux, equivalents on other systems).
# This replaces the hardcoded -D_GNU_SOURCE in the old Makefile and is the
# portable autoconf-recommended approach.
AC_USE_SYSTEM_EXTENSIONS

# clock_gettime lives in librt on older glibc (< 2.17); in libc on modern ones.
AC_SEARCH_LIBS([clock_gettime], [rt])

# posix_memalign used for aligned buffer allocation
AC_CHECK_FUNCS([posix_memalign])

# posix_fadvise is Linux-only; code guards it with #ifndef __APPLE__ already
AC_CHECK_FUNCS([posix_fadvise])

# vasprintf is a GNU/BSD extension used in verbose.c
AC_CHECK_FUNCS([vasprintf])

# get_current_dir_name is a GNU extension; code falls back to getcwd on macOS
AC_CHECK_FUNCS([get_current_dir_name])

AC_CONFIG_HEADERS([config.h])
AC_OUTPUT([
    Makefile
    src/Makefile
    man1/Makefile
])
