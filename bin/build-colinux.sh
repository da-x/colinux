#!/bin/sh

source ./build-common.sh

# See ./commom.sh (using cross tools for target, such strip
PATH="$PREFIX/$TARGET/bin:$PATH"

# Need Variable in make
export COLINUX_TARGET_KERNEL_PATH

configure_colinux_daemons()
{
	cd "$TOPDIR/../src"
	echo "Compiling colinux (daemons)"
	make colinux &> make.log
	if test $? -ne 0; then
	        echo "colinux make failed"
	        echo "   - log available: $TOPDIR/../src/make.log"
	        exit 1
	fi
	make bridged_net_daemon &> make-bridged-net-daemon.log
	if test $? -ne 0; then
	        echo "bridged net daemon make failed"
	        echo "   - log available: $TOPDIR/../src/make-bridged-net-daemon.log"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_colinux_daemons()
{
	cd "$TOPDIR/../src"
	echo "Installing colinux (daemons) to $PREFIX/dist"
	cp -a colinux/os/winnt/user/conet-daemon/colinux-net-daemon.exe "$PREFIX/dist"
	cp -a colinux/os/winnt/user/console/colinux-console-fltk.exe "$PREFIX/dist"
	cp -a colinux/os/winnt/user/console-nt/colinux-console-nt.exe "$PREFIX/dist"
	cp -a colinux/os/winnt/user/daemon/colinux-daemon.exe "$PREFIX/dist"
	cp -a colinux/os/winnt/build/linux.sys "$PREFIX/dist"
	# Strip debug information form distribution executable
	strip $PREFIX/dist/*.exe
	cd "$TOPDIR"
}

build_colinux_daemons()
{
	# nothing to do for download
	test "$1" = "--download-only" && exit 0
	configure_colinux_daemons
	install_colinux_daemons
}

# ALL
build_colinux_daemons $1
