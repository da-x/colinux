#!/bin/sh

source ./build-common.sh

# See ./commom.sh (using cross tools for target, such strip
PATH="$PREFIX/$TARGET/bin:$PATH"

# Need Variable in make
export COLINUX_TARGET_KERNEL_PATH
export COLINUX_INSTALL_DIR

compile_colinux_daemons()
{
	echo "Compiling colinux (daemons)"
	cd "$TOPDIR/../src"
	make colinux >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "colinux make failed"
	cd "$TOPDIR"
}

install_colinux_daemons()
{
	echo "Installing colinux (daemons) to $COLINUX_INSTALL_DIR/"
	cd "$TOPDIR/../src"
	make install
	test $? -ne 0 && error_exit 1 "colinux install failed"
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
