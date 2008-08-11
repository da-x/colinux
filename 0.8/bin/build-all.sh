#!/bin/sh

# This script calls sequenzly all others scripts.
#
# Options can ones of (only ones, or none):
#  --rebuild		Rebuild all, without checking old targed files.
#			Disable md5sum checking, untar and patch source.
#			Overwrites kernel-source! (with backup)
#  --download-only	Download all source files, no compile
#  --installer		Create Windows installer

. ./build-common.sh

./build-cross.sh $1 && \
./build-colinux-libs.sh $1 && \
./build-kernel.sh $1 && \
./build-colinux.sh $1 && \
echo "Build-all $1 DONE"
