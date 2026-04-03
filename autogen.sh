#!/bin/sh
# Run this script once after cloning to generate the configure script
# and Makefile.in templates. Requires autoconf and automake.
set -e
autoreconf --install --force
echo ""
echo "Done. Now run: ./configure && make"
