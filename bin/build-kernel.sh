#!/bin/sh

#
# Options (only ones of):
#  --download-only	Download all source files, no compile
#  --no-download	Never download, extract or patch source. Mostly used,
#			on kernel developers, to disable the automatic untar.
#			md5sum will not check.
#  --rebuild-all	Rebuild all, without checking old targed files.
#			Disable md5sum. untar and patch source.
#			Overwrite all old source!

source ./build-common.sh

# Remember: Please update also conf/kernel-config, if changing kernel version!

KERNEL=linux-$KERNEL_VERSION
KERNEL_URL=http://www.kernel.org/pub/linux/kernel/$KERNEL_DIR
KERNEL_ARCHIVE=$KERNEL.tar.bz2

# Developer private patchfile. Used after maintainer patch,
# use also for manipulate .config. Sored in directory patch/.
PRIVATE_PATCH=linux-private.patch

CHECKSUM_FILE=$SRCDIR/.build-kernel.md5

CO_VERSION=`cat ../src/colinux/VERSION`
COMPLETE_KERNEL_NAME=$KERNEL_VERSION-co-$CO_VERSION

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$KERNEL_ARCHIVE" "$KERNEL_URL"
}

check_md5sums()
{
	echo "Check md5sum"
	cd "$TOPDIR/.."
	if md5sum --check $CHECKSUM_FILE >>$COLINUX_BUILD_LOG 2>&1 ; then
		echo "Skip vmlinux,modules-$COMPLETE_KERNEL_NAME.tar.gz"
		echo " - already installed on $COLINUX_INSTALL_DIR"
		exit 0
	fi
	cd "$TOPDIR"
}

create_md5sums()
{
	echo "Create md5sum"
	cd "$TOPDIR/.."
	md5sum --binary \
	    patch/linux \
	    conf/linux-config \
	    $COLINUX_INSTALL_DIR/modules-$COMPLETE_KERNEL_NAME.tar.gz \
	    $COLINUX_INSTALL_DIR/vmlinux \
	    $COLINUX_TARGET_KERNEL_PATH/vmlinux \
	    $COLINUX_TARGET_KERNEL_PATH/.config \
	    > $CHECKSUM_FILE
	test $? -ne 0 && error_exit 1 "can not create md5sum"
	if [ -f patch/$PRIVATE_PATCH ]; then
		md5sum --binary patch/$PRIVATE_PATCH >> $CHECKSUM_FILE
	fi
	cd "$TOPDIR"
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
	bzip2 -dc "$SRCDIR/$KERNEL_ARCHIVE" | tar x
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
	test $? -ne 0 && error_exit 1 "$KERNEL_VERSION patch failed"
	cd "$TOPDIR"
}

configure_kernel()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	# A minor hack for now.  Allowing linux config to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	cp "$TOPDIR/../conf/linux-config" "$TOPDIR/../conf/linux-$KERNEL_VERSION-config"
	cp "$TOPDIR/../conf/linux-$KERNEL_VERSION-config" .config

	# Last chance to add private things, such local config
	if [ -f $TOPDIR/../patch/$PRIVATE_PATCH ]; then
		echo "Private patch $PRIVATE_PATCH"
		patch -p1 < "$TOPDIR/../patch/$PRIVATE_PATCH"
	        test $? -ne 0 && error_exit 1 "patch/$PRIVATE_PATCH patch failed"
	fi

	echo "Configuring Kernel $KERNEL_VERSION"
	make silentoldconfig >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION config failed (check 'make oldconfig' on kerneltree)"

	echo "Dep Kernel $KERNEL_VERSION"
	make dep >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION dep failed"
	cd "$TOPDIR"
}

compile_kernel()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Making Kernel $KERNEL_VERSION"
	make vmlinux >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION make failed"
	cd "$TOPDIR"
}

install_kernel()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Installing Kernel $KERNEL_VERSION in $COLINUX_INSTALL_DIR"
	mkdir -p "$COLINUX_INSTALL_DIR"
	cp -a vmlinux $COLINUX_INSTALL_DIR
	cd "$TOPDIR"
}

#
# Modules
#

compile_modules()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Making Modules $KERNEL_VERSION"
	make INSTALL_MOD_PATH=`pwd`/_install modules modules_install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION make modules failed"
	cd "$TOPDIR"
}

# Create compressed tar archive with path for extracting direct on the
# root of fs, lib/modules with full version of kernel and colinux.
install_modules()
{
	cd "$TOPDIR"
	cd $COLINUX_TARGET_KERNEL_PATH/_install
	echo "Installing Modules $KERNEL_VERSION in $COLINUX_INSTALL_DIR"
	mkdir -p "$COLINUX_INSTALL_DIR"
	tar cfz $COLINUX_INSTALL_DIR/modules-$COMPLETE_KERNEL_NAME.tar.gz lib/modules/$COMPLETE_KERNEL_NAME
	if test $? -ne 0; then
	        echo "Kernel $COMPLETE_KERNEL_NAME modules install failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

build_kernel()
{
	# Full user control for compile (for kernel developers)
	if [ "$1" != "--no-download" -a "$COLINUX_KERNEL_UNTAR" != "no" ]; then
          download_files
	  # Only Download? Than ready.
	  test "$1" = "--download-only" && exit 0

	  # do not check files, if rebuild forced
	  test "$1" = "--rebuild-all" || check_md5sums

	  # Extract, patch and configure Kernel
	  extract_kernel
	  patch_kernel
	  configure_kernel
	fi

	# Build and install Kernel vmlinux
	compile_kernel
	install_kernel

	# Build and install Modules
	compile_modules
	install_modules

	create_md5sums
}

build_kernel $1
