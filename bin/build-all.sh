#!/bin/sh

#
# This script builds the mingw32 cross compiler on Linux,
# and patches the w32api package.
#
# It also downloads, compiles, and installs FLTK and mxml.
# 
# - Dan Aloni <da-x@colinux.org>
#

./build-cross.sh &&
./build-colinux-libs.sh
