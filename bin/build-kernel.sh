#!/bin/sh -x

source ./build-common.sh

# See ./commom.sh

KERNEL_DIR=v2.6
KERNEL_VERSION=2.6.7
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
	cd "$TOPDIR"
	rm -rf $COLINUX_TARGET_KERNEL_PATH
	cd "$SRCDIR"
	rm -rf "$KERNEL"
	echo "Extracting Kernel $KERNEL_VERSION"
	bzip2 -dc "$SRCDIR/$KERNEL_ARCHIVE" | tar xf -
	cd "$TOPDIR"
}

patch_kernel()
{
	cd "$TOPDIR"
        ln -s "$SRCDIR/$KERNEL" "$COLINUX_TARGET_KERNEL_PATH"
	cd "$COLINUX_TARGET_KERNEL_PATH"
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
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Configuring Kernel $KERNEL_VERSION"
	# A minor hack for now.  Allowing linux config to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	cp "$TOPDIR/../conf/linux-config" "$TOPDIR/../conf/linux-$KERNEL_VERSION-config"
	cp "$TOPDIR/../conf/linux-$KERNEL_VERSION-config" .config
	make oldconfig &> configure.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION configure failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/configure.log"
	        exit 1
	fi
	echo "Making Kernel $KERNEL_VERSION"
	make dep &> make-dep.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION make dep failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/make-dep.log"
	        exit 1
	fi
	make vmlinux &> make.log
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION make failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/make.log"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_kernel()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Installing Kernel $KERNEL_VERSION in $PREFIX/dist"
	mkdir -p "$PREFIX/dist"
	cp -a vmlinux $PREFIX/dist
	
	# It would be nice to install kernel modules
	# and then tar them up here.

	cd "$TOPDIR"
}

build_kernel()
{
        download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0
	extract_kernel
	patch_kernel
	configure_kernel
	install_kernel
}

build_kernel $1
