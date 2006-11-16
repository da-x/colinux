#!/bin/sh

# Build colinux daemons from cross platform. Read doc/building

. ./build-common.sh

if [ "$COLINUX_HOST_OS" == "linux" ]
then
	# This script can't build for linux as host
	echo "error: $0 is only to build winnt. Please 'make' in toplevel!"
	exit 1
fi

# See build-commom.sh (using cross tools for target, such strip
PATH="$PREFIX/$TARGET/bin:$PATH"

# Need Variable in make
export COLINUX_TARGET_KERNEL_PATH
export COLINUX_INSTALL_DIR

# nothing to do for download
test "$1" = "--download-only" && exit 0

cd "$TOPDIR/../src"

echo "Compiling colinux (daemons)"
make colinux || { echo "colinux make failed"; exit 1; }

echo "(Pre)Installing colinux (daemons) to $COLINUX_INSTALL_DIR/"
make install || { echo "colinux (pre)install failed"; exit 1; }

# Uncomment if you have WINE installed with NullSoft Installer 
#  System v2.05 or later and would like an Windows based Installer
#  created.
# echo "Create installer"
# make installer || { echo "colinux installer failed"; exit 1; }

cd $TOPDIR

exit 0
