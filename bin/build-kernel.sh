#!/bin/sh

source ./build-common.sh

# See ./commom.sh

# Remember: Please update also conf/kernel-config, if changing kernel version!

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
	make silentoldconfig &> configure.log
	if test $? -ne 0; then
		tail configure.log
	        echo "Kernel $KERNEL_VERSION configure failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/configure.log"

		# Ask user for new things
		echo "If config to old for kernel-Version?"
		echo "   Run 'make oldconfig' on kerneltree, than"
		echo "   copy .config as conf/linux-config for colinux and"
		echo "   run build again."
	        exit 1
	fi

	echo "Dep Kernel $KERNEL_VERSION"
	make dep &> make-dep.log
	if test $? -ne 0; then
		tail make-dep.log
	        echo "Kernel $KERNEL_VERSION make dep failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/make-dep.log"
	        exit 1
	fi
	cd "$TOPDIR"
}

compile_kernel()
{
	cd "$TOPDIR"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Making Kernel $KERNEL_VERSION"
	make vmlinux &> make.log
	if test $? -ne 0; then
		tail make.log
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
	echo "Installing Kernel $KERNEL_VERSION in $INSTALL_DIR"
	mkdir -p "$INSTALL_DIR"
	cp -a vmlinux $INSTALL_DIR
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
	make INSTALL_MOD_PATH=`pwd`/_install modules modules_install &> make-modules.log
	if test $? -ne 0; then
		tail make-modules.log
	        echo "Kernel $KERNEL_VERSION make modules failed"
	        echo "   - log available: $COLINUX_TARGET_KERNEL_PATH/make-modules.log"
	        exit 1
	fi
	cd "$TOPDIR"
}

# Create compressed tar archive with path for extracting direct on the
# root of fs, lib/modules with full version of kernel and colinux.
install_modules()
{
	cd "$TOPDIR"
	CO_VERSION=`cat ../src/colinux/VERSION`
	NAME=$KERNEL_VERSION-co-$CO_VERSION
	cd $COLINUX_TARGET_KERNEL_PATH/_install
	echo "Installing Modules $KERNEL_VERSION in $INSTALL_DIR"
	mkdir -p "$INSTALL_DIR"
	tar cfz $INSTALL_DIR/modules-$NAME.tar.gz lib/modules/$NAME
	if test $? -ne 0; then
	        echo "Kernel $KERNEL_VERSION-co-$CO_VERSION modules install failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

build_kernel()
{
        download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0

	# Extract, patch and configure Kernel
	extract_kernel
	patch_kernel
	configure_kernel

	# Build and install Kernel vmlinux
	compile_kernel
	install_kernel

	# Build and install Modules
	compile_modules
	install_modules
}

build_kernel $1
