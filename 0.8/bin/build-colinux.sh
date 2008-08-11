#!/bin/sh

# Builds daemons, ZIP package and Installer. Please read doc/building.
#
# Run './build-colinux.sh --installer', if you have WINE installed
#  with NullSoft Installer System v2.05 or later and
#  would like an Windows based Installer created.

. ./build-common.sh

# nothing to do for download
test "$1" = "--download-only" && exit 0

# Need Variables in make
export COLINUX_TARGET_KERNEL_SOURCE
export COLINUX_TARGET_KERNEL_BUILD
export COLINUX_TARGET_KERNEL_PATH

echo "Compiling colinux (daemons)"
cd $TOPDIR/src
if ! make colinux
then
	echo "colinux make daemons failed"
	exit 1
fi

echo "Build premaid"
cd $TOPDIR/src/colinux/os/winnt/user/install && . ./premaid.sh
cd $BINDIR

# Create ZIP files
build_package

if [ "$1" = "--installer" ]
then
	echo "Create installer"
	cd $TOPDIR/src
	if ! make installer
	then
		echo "colinux make installer failed"
		exit 1
	fi
fi
