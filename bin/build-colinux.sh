#!/bin/sh

source ./build-common.sh

# See ./commom.sh

KERNEL_DIR=v2.4 
KERNEL_VERSION=2.4.26 
KERNEL=linux-$KERNEL_VERSION 
KERNEL_URL=http://www.kernel.org/pub/linux/kernel/$KERNEL_DIR 
KERNEL_ARCHIVE=$KERNEL.tar.bz2

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$KERNEL_ARCHIVE" "$KERNEL_URL"
}

#
# Kernel
#

extract_kernel()
{
	cd "$SRCDIR"
	rm -rf $TOPDIR/../../linux
	rm -rf "$KERNEL"
	echo "Extracting Kernel $KERNEL_VERSION"
	bzip2 -dc "$SRCDIR/$KERNEL_ARCHIVE" | tar xf -
	cd "$TOPDIR"
}

patch_kernel()
{
	cd "$SRCDIR"
        ln -s "$SRCDIR/$KERNEL" "$TOPDIR/../../linux"
	cd "$TOPDIR/../../linux"
	# A minor hack for now.  allowing linux patch to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	cp "$TOPDIR/../patch/linux" "$TOPDIR/../patch/linux-$KERNEL_VERSION.diff"
	patch -p1 < "$TOPDIR/../patch/linux-$KERNEL_VERSION.diff"
	if test $? -ne 0; then
	        echo "$KERNEL_VERSION patch failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

configure_kernel()
{
	cd "$TOPDIR/../../linux"
	echo "Configuring Kernel $KERNEL_VERSION"
	# A minor hack for now.  Allowing linux config to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	cp "$TOPDIR/../conf/linux-config" "$TOPDIR/../conf/linux-$KERNEL_VERSION-config"
	cp "$TOPDIR/../conf/linux-$KERNEL_VERSION-config" .config
	make oldconfig &> configure.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION configure failed"
	        exit 1
	fi
	echo "Making Kernel $KERNEL_VERSION"
	make dep &> make-dep.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION make dep failed"
	        exit 1
	fi
	make vmlinux &> make.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION make failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_kernel()
{
	cd "$TOPDIR/../../linux"
	echo "Installing Kernel $KERNEL_VERSION"
	cp -a vmlinux $PREFIX/$TARGET/bin
	
	# It would be nice to install kernel modules
	# and then tar them up here.

	cd "$TOPDIR"
}

configure_colinux_daemons()
{
	cd "$TOPDIR/../src"
	echo "Configuring colinux (daemons)"
	make colinux &> make.log
	if test $? -ne 0; then
	        echo "colinux make failed"
	        exit 1
	fi
	make bridged_net_daemon &> make-bridged-net-daemon.log
	if test $? -ne 0; then
	        echo "bridged net daemon make failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_colinux_daemons()
{
	cd "$TOPDIR/../src"
	echo "Installing colinux (daemons)"
	cp -a colinux/os/winnt/user/conet-daemon/colinux-net-daemon.exe "$PREFIX/$TARGET/bin"
	cp -a colinux/os/winnt/user/console/colinux-console-fltk.exe "$PREFIX/$TARGET/bin"
	cp -a colinux/os/winnt/user/console-nt/colinux-console-nt.exe "$PREFIX/$TARGET/bin"
	cp -a colinux/os/winnt/user/daemon/colinux-daemon.exe "$PREFIX/$TARGET/bin"
	cp -a colinux/os/winnt/build/linux.sys "$PREFIX/$TARGET/bin"
	cd "$TOPDIR"
}

build_kernel()
{
	extract_kernel
	patch_kernel
	configure_kernel
	install_kernel
}

build_colinux_daemons()
{
	configure_colinux_daemons
	install_colinux_daemons
}

# ALL

build_colinux()
{
        download_files
	build_kernel
	build_colinux_daemons
}

build_colinux
