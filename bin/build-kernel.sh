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

. ./build-common.sh

# Use different gcc for kernel, if defined
if [ -n "$COLINUX_GCC_GUEST_TARGET" ]
then
	# Default path, if empty
	test -z "$COLINUX_CGG_GUEST_PATH" && \
		COLINUX_GCC_GUEST_PATH="$PREFIX/$COLINUX_GCC_GUEST_TARGET/bin"
	export PATH="$PATH:$COLINUX_GCC_GUEST_PATH"
	export CROSS_COMPILE="${COLINUX_GCC_GUEST_TARGET}-"
fi

download_files()
{
	download_file "$KERNEL_ARCHIVE" "$KERNEL_URL"
}

# If md5sum fails or not exist, kernel must compile
check_md5sums()
{
	echo -n "Check kernel and modules: "
	cd "$TOPDIR"

	if [ ! -f "$COLINUX_TARGET_KERNEL_BUILD/vmlinux" ]
	then
		echo "vmlinux don't exist, build it now"
		return
	fi

	if md5sum -c $KERNEL_CHECKSUM >/dev/null 2>&1
	then
		echo "Skip $COMPLETE_KERNEL_NAME"
		echo " - already done in $COLINUX_TARGET_KERNEL_BUILD"
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
	    conf/linux-$KERNEL_VERSION-config \
	    "$COLINUX_TARGET_KERNEL_BUILD/.config" \
	    "$COLINUX_TARGET_KERNEL_BUILD/vmlinux" \
	    "$COLINUX_TARGET_MODULE_PATH/lib/modules/$COMPLETE_KERNEL_NAME/modules.dep" \
	    > $KERNEL_CHECKSUM \
	|| error_exit 10 "can not create md5sum"

	# Md5sums for patches
	cd patch
	for name in `cat series-$KERNEL_VERSION` config-*.patch
	do
		if [ -e "$name" ]
		then
			md5sum -b $name >> $KERNEL_CHECKSUM
		fi
	done

	cd "$BINDIR"
}

#
# Kernel
#

extract_kernel()
{
	echo "Extracting Kernel $KERNEL_VERSION"

	# Backup users modifired source
	if [ -d "$COLINUX_TARGET_KERNEL_SOURCE" -a ! -d "$COLINUX_TARGET_KERNEL_SOURCE.bak" ]
	then
		echo "Backup old kernel source to"
		echo " $COLINUX_TARGET_KERNEL_SOURCE.bak"
		mv "$COLINUX_TARGET_KERNEL_SOURCE" "$COLINUX_TARGET_KERNEL_SOURCE.bak"
		cp -p "$COLINUX_TARGET_KERNEL_BUILD/.config" "$COLINUX_TARGET_KERNEL_SOURCE.bak/config"
	fi

	mkdir -p "$COLINUX_TARGET_KERNEL_SOURCE-tmp" || exit 1
	rm -rf "$COLINUX_TARGET_KERNEL_SOURCE" "$COLINUX_TARGET_KERNEL_BUILD"
	bzip2 -dc "$SOURCE_DIR/$KERNEL_ARCHIVE" | tar x -C "$COLINUX_TARGET_KERNEL_SOURCE-tmp" \
	|| error_exit 10 "$KERNEL_VERSION extract failed"
	mv "$COLINUX_TARGET_KERNEL_SOURCE-tmp/$KERNEL" "$COLINUX_TARGET_KERNEL_SOURCE" || exit 1
	rm -rf "$COLINUX_TARGET_KERNEL_SOURCE-tmp"
}

emulate_quilt_push()
{
	while read name level
	do
		if [ -e "patches/$name" ]
		then
			echo "reading $name"
			test -z "$level" && level="-p1"
			patch $level -f < "patches/$name" \
			|| error_exit 10 "$name patch failed"
		fi
	done < series
}

# Standard patch, user patches...
patch_kernel_source()
{
	cd "$COLINUX_TARGET_KERNEL_SOURCE"
	test -f "series"  || ln -s "$TOPDIR/patch/series-$KERNEL_VERSION" "series"
	test -f "patches" || ln -s "$TOPDIR/patch" "patches"

	if quilt --version >/dev/null
	then
		# use quilt for patching, don't trust users settings
		unset QUILT_COMMAND_ARGS
		unset QUILT_PATCHES
		quilt --quiltrc /dev/null push -a \
		|| error_exit 10 "quilt failed"
	else
		# Fallback without quilt
		emulate_quilt_push
	fi

	# Copy coLinux Version into kernel localversion
	echo "-co-$CO_VERSION" > localversion-cooperative
	cd -
}

patch_kernel_build()
{
	mkdir -p "$COLINUX_TARGET_KERNEL_BUILD" || exit 1
	# A minor hack for now.  Allowing linux config to be 'version specific' 
	#  in the future, but keeping backwards compatability.
	if [ ! -f "$TOPDIR/conf/linux-$KERNEL_VERSION-config" ]; then
		ln -s "linux-config" "$TOPDIR/conf/linux-$KERNEL_VERSION-config"
	fi
	cp "$TOPDIR/conf/linux-$KERNEL_VERSION-config" "$COLINUX_TARGET_KERNEL_BUILD/.config"

	# patches on config in buildir
	for name in "$TOPDIR"/patch/config-*.patch
	do
		echo "reading $name"
		patch -p1 -f -d "$COLINUX_TARGET_KERNEL_BUILD" < $name \
		|| error_exit 10 "$name patch failed"
	done
}

configure_kernel()
{
	# Is this a patched kernel?
	if [ ! -f "$COLINUX_TARGET_KERNEL_SOURCE/include/linux/cooperative.h" ]
	then
		cat <<EOF >&2
Hups: Missing cooperative.h in kernel source!

Please verify setting of these variables:
COLINUX_TARGET_KERNEL_SOURCE=$COLINUX_TARGET_KERNEL_SOURCE
BUILD_DIR/KERNEL=$BUILD_DIR/$KERNEL

\$COLINUX_TARGET_KERNEL_SOURCE should the same as \$BUILD_DIR/\$KERNEL
EOF
		exit 1
	fi

	echo "Configuring Kernel $KERNEL_VERSION"
	cd "$COLINUX_TARGET_KERNEL_SOURCE" || exit 1
	make O="$COLINUX_TARGET_KERNEL_BUILD" silentoldconfig >>$COLINUX_BUILD_LOG 2>&1 \
	|| error_exit 1 "Kernel $KERNEL_VERSION config failed (check 'make oldconfig' on kerneltree)"
}

compile_kernel()
{
	echo "Making Kernel $KERNEL_VERSION"
	cd "$COLINUX_TARGET_KERNEL_BUILD" || exit 1
	make vmlinux >>$COLINUX_BUILD_LOG 2>&1 \
	|| error_exit 1 "Kernel $KERNEL_VERSION make failed"
}

#
# Modules
#

compile_modules()
{
	echo "Making Modules $KERNEL_VERSION"
	cd "$COLINUX_TARGET_KERNEL_BUILD" || exit 1

	#Fall back for older config
	test -z "$COLINUX_DEPMOD" && COLINUX_DEPMOD=/sbin/depmod

	make \
	    INSTALL_MOD_PATH="$COLINUX_TARGET_MODULE_PATH" \
	    DEPMOD=$COLINUX_DEPMOD \
	    modules modules_install >>$COLINUX_BUILD_LOG 2>&1 \
	|| error_exit 1 "Kernel $KERNEL_VERSION make modules failed"
}

build_kernel()
{
	echo "log: $COLINUX_BUILD_LOG"
	mkdir -p `dirname $COLINUX_BUILD_LOG`

	# Full user control for compile (kernel developers)
	if [ "$1" != "--no-download" -a "$COLINUX_KERNEL_UNTAR" = "yes" \
	     -o ! -s "$COLINUX_TARGET_KERNEL_SOURCE/include/linux/cooperative.h" ]; then

		# do not check files, if rebuild forced
		test "$1" = "--rebuild" || check_md5sums

		download_files
		# Only Download? Than ready.
		test "$1" = "--download-only" && exit 0

		# Extract, patch and configure Kernel
		extract_kernel
		patch_kernel_source
	fi

	if [ ! -f $COLINUX_TARGET_KERNEL_BUILD/.config ]; then
		patch_kernel_build
		configure_kernel
	fi

	# Build Kernel vmlinux
	compile_kernel

	# Build and install Modules
	compile_modules

	create_md5sums
}

build_kernel $1
