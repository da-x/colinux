#!/bin/sh

#
# This script builds the mingw32 cross compiler on Linux,
# and patches the w32api package.
#
# It also downloads, compiles, and installs FLTK and mxml.
# 
# - Dan Aloni <da-x@colinux.org>
#
# Options (only ones of):
#  --download-only	Download all source files, no compile
#  --no-download	Never download, extract or patch source. Mostly used,
#			on kernel developers, to disable the automatic untar.
#			md5sum will not check.
#  --rebuild-all	Rebuild all, without checking old targed files.
#			Disable md5sum. untar and patch source.
#			Overwrite all old source!

./build-cross.sh $1 && \
./build-colinux-libs.sh $1 && \
./build-kernel.sh $1 && \
./build-colinux.sh $1 && \
echo "Build-all $1 DONE"
