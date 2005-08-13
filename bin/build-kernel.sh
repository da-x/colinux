#!/bin/sh

#
# Options (only ones of):
#  --download-only	Download all source files, no compile
#  --no-download	Never download, extract or patch source. Mostly used,
#			on kernel developers, to disable the automatic untar.
#			md5sum will not check.
#  --rebuild		Rebuild, without checking old targed files.
#			Disable md5sum. untar and patch source.
#			Overwrite all old source!

. build-common.sh

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$KERNEL_ARCHIVE" "$KERNEL_URL"
}

# If md5sum fails or not exist, kernel must compile
check_md5sums()
{
	echo -n "Check kernel and modules: "
	cd "$TOPDIR"
	if md5sum -c $KERNEL_CHECKSUM >/dev/null 2>&1
	then
		echo "Skip $COMPLETE_KERNEL_NAME"
		echo " - already done in $COLINUX_TARGET_KERNEL_PATH"
		exit 0
	fi
	echo "MD5sum don't match, rebuilding"
}

# Create md5sum over binary and key source
create_md5sums()
{
	echo "Create md5sum"
	cd "$TOPDIR"
	md5sum -b \
	    src/colinux/VERSION \
	    $KERNEL_PATCH \
	    conf/linux-$KERNEL_VERSION-config \
	    $COLINUX_TARGET_KERNEL_PATH/.config \
	    $COLINUX_TARGET_KERNEL_PATH/vmlinux \
	    $COLINUX_TARGET_MODULE_PATH/lib/modules/$COMPLETE_KERNEL_NAME/modules.dep \
	    > $KERNEL_CHECKSUM
	test $? -ne 0 && error_exit 10 "can not create md5sum"

	if [ -f $PRIVATE_PATCH ]
	then
		md5sum -b $PRIVATE_PATCH >> $KERNEL_CHECKSUM
	fi
	cd "$BINDIR"
}

#
# Kernel
#

extract_kernel()
{
	echo "Extracting Kernel $KERNEL_VERSION"
	cd "$BUILD_DIR"
	# Backup users modifired source
	if [ -d "$KERNEL" -a ! -d "$KERNEL.bak" ]
	then
		mv "$KERNEL" "$KERNEL.bak"
	fi
	rm -rf "$KERNEL"
	bzip2 -dc "$SRCDIR/$KERNEL_ARCHIVE" | tar x
	test $? -ne 0 && error_exit 10 "$KERNEL_VERSION extract failed"
}

patch_kernel()
{
	cd "$BUILD_DIR/$KERNEL"

	# A minor hack for now.  Allowing linux config to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	if [ -f "$TOPDIR/conf/linux-config" ]; then
		cp "$TOPDIR/conf/linux-config" "$TOPDIR/conf/linux-$KERNEL_VERSION-config"
	fi
	cp "$TOPDIR/conf/linux-$KERNEL_VERSION-config" .config

	# Standard patch, user patches, alphabetically sort by name
	# Last chance to add private things, such local config
	for name in $TOPDIR/$KERNEL_PATCH $TOPDIR/patch/linux-*.patch
	do
		if [ -f $name ]
		then
			echo "reading $name"
			patch -p1 < $name
			test $? -ne 0 && error_exit 10 "$name patch failed"
		fi
	done

	# Copy coLinux Version into kernel localversion
	echo "-co-$CO_VERSION" > localversion-cooperative
}

configure_kernel()
{
	# Is this a patched kernel?
	if [ ! -f $COLINUX_TARGET_KERNEL_PATH/include/linux/cooperative.h ]
	then
		echo -e "\nHups: Missing cooperative.h in kernel source!\n"
		echo "Please verify setting of these variables:"
		echo "COLINUX_TARGET_KERNEL_PATH=$COLINUX_TARGET_KERNEL_PATH"
		echo "BUILD_DIR/KERNEL=$BUILD_DIR/$KERNEL"
		echo
		echo "\$COLINUX_TARGET_KERNEL_PATH should the same as \$BUILD_DIR/\$KERNEL"
		exit 1
	fi

	cd "$COLINUX_TARGET_KERNEL_PATH"
	echo "Configuring Kernel $KERNEL_VERSION"
	make silentoldconfig
	test $? -ne 0 && error_exit 10 \
	    "Kernel $KERNEL_VERSION config failed (check 'make oldconfig' on kerneltree)"
}

compile_kernel()
{
	echo "Making Kernel $KERNEL_VERSION"
	cd "$COLINUX_TARGET_KERNEL_PATH"
	make vmlinux >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION make failed"
}

#
# Modules
#

compile_modules()
{
	echo "Making Modules $KERNEL_VERSION"
	cd "$COLINUX_TARGET_KERNEL_PATH"

	#Fall back for older config
	test -z "$COLINUX_DEPMOD" && COLINUX_DEPMOD=/sbin/depmod

	make \
	    INSTALL_MOD_PATH=$COLINUX_TARGET_MODULE_PATH \
	    DEPMOD=$COLINUX_DEPMOD \
	    modules modules_install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "Kernel $KERNEL_VERSION make modules failed"
}

build_kernel()
{
	# Full user control for compile (kernel developers)
	if [ "$1" != "--no-download" -a "$COLINUX_KERNEL_UNTAR" != "no" \
	     -o ! -f $COLINUX_TARGET_KERNEL_PATH/include/linux/cooperative.h ]; then

	  # do not check files, if rebuild forced
	  test "$1" = "--rebuild" || check_md5sums

	  download_files
	  # Only Download? Than ready.
	  test "$1" = "--download-only" && exit 0

	  # Extract, patch and configure Kernel
	  extract_kernel
	  patch_kernel
	  configure_kernel
	fi

	# Build Kernel vmlinux
	compile_kernel

	# Build and install Modules
	compile_modules

	create_md5sums
}

build_kernel $1
