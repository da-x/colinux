#!/bin/sh

#
# This script downloads all files for build the mingw32 cross compiler on Linux,
# patches, FLTK and mxml.
#
# This checks also all needed files in download direcory.
# 
# - Henry Nestler <henry@bigfoot.de>
#

./build-cross.sh	--download-only &&
./build-colinux-libs.sh	--download-only &&
./build-kernel.sh	--download-only &&
./build-colinux.sh	--download-only
