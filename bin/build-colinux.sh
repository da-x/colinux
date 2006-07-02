#!/bin/sh

# Build colinux daemons from cross platform. Read doc/building

. ./build-common.sh

# nothing to do for download
test "$1" = "--download-only" && exit 0

# Need Variables in make
export COLINUX_TARGET_KERNEL_SOURCE
export COLINUX_TARGET_KERNEL_BUILD
export COLINUX_TARGET_KERNEL_PATH

echo "Compiling colinux (daemons)"
cd $TOPDIR/src
if make colinux
then
	echo "colinux make daemons failed"
	exit 1
fi
cd $BINDIR

# linux as host and ends here
test "$COLINUX_HOST_OS" = "linux" && exit 0

# Set from "false" to "true" If you have WINE installed
#  with NullSoft Installer System v2.05 or later and
#  would like an Windows based Installer created.
#
if false
then
	echo "Build premaid"
	cd $TOPDIR/src/colinux/os/winnt/user/install && . premaid.sh

	echo "Create installer"
	cd $TOPDIR/src
	if make installer
	then
		echo "colinux make installer failed"
		exit 1
	fi
fi
