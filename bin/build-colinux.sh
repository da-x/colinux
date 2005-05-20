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

compile_colinux_daemons()
{
	echo "Compiling colinux (daemons)"
	cd "$TOPDIR/../src"
	make colinux || { echo "colinux make failed"; exit 1; }
	cd "$TOPDIR"
}

install_colinux_daemons()
{
	echo "Installing colinux (daemons) to $COLINUX_INSTALL_DIR/"
	cd "$TOPDIR/../src"
	make install || { echo "colinux install failed"; exit 1; }
	cd $TOPDIR
}

build_colinux_daemons()
{
	# nothing to do for download
	test "$1" = "--download-only" && exit 0
	compile_colinux_daemons
	install_colinux_daemons
}

# ALL
build_colinux_daemons $1
