#!/bin/sh

# Build cross platform mingw32.

. ./build-common.sh

settings64()
{
	# Get host machine type and select the right download
	MACHINE=`uname -m`
	test "$MACHINE" = "x86_64" || MACHINE="i686"

	# Download links and names
	# x86_64 (64 bit): MINGW_PACK="mingw-w64-bin_x86_64-linux_20100711_sezero.tar.gz"
	# i686   (32 bit): MINGW_PACK="mingw-w64-bin_i686-linux_20100711_sezero.tar.gz"
	#
	MINGW_DATE="20100711"
	MINGW_BDIR="W64_162041"
	MINGW_PACK="mingw-w64-bin_$MACHINE-linux_${MINGW_DATE}_sezero.tar.gz"
	MINGW_URL="http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/sezero_${MINGW_DATE}"
	MINGW_PACK_URL="$MINGW_URL/$MINGW_PACK/download"

	# Update headers and libs
	UPDATE_REV="2951"
	UPDATE_PACK="sezero_${MINGW_DATE}_w64_runtime_update_$UPDATE_REV.zip"
	UPDATE_PACK_URL="$MINGW_URL/$UPDATE_PACK/download"

	# Pinned state for DDK headers
	DDK_REV="2960"
	DDK_SVN="http://mingw-w64.svn.sourceforge.net/svnroot/mingw-w64/experimental/ddk_test"
	DDK_MIRROR="$DOWNLOADS/MinGW64-ddk_test.svn"
}

download64()
{
	echo "Building ToolChain is not supported. Will download a prebuild now. Needs svn client for this."
	mkdir -p $DOWNLOADS

	# Prebuild GCC MinGW for Linux (43M download, 227M used)
	test -f $DOWNLOADS/$MINGW_PACK || \
	 wget $MINGW_PACK_URL -P $DOWNLOADS

	# Last update
	test -f $DOWNLOADS/$UPDATE_PACK || \
	 wget $UPDATE_PACK_URL -P $DOWNLOADS

	# Get ddk/psdk headers from MinGW testing (7M used)
	# http://article.gmane.org/gmane.comp.gnu.mingw.w64.general/1079
	test -d $DDK_MIRROR || \
	 svn checkout -r $DDK_REV $DDK_SVN $DDK_MIRROR && \
	 svn update -r $DDK_REV $DDK_SVN
}

unpack64()
{
	if [ ! -d $PREFIX/$TARGET/include ]
	then
		# "sezero" does not tar'ed standard PREFIX. So, unpack somewhere and move
		echo "Unpack MinGW binary..."
		mkdir -p $PREFIX/sezero_tmp
		tar xzf $DOWNLOADS/$MINGW_PACK -C $PREFIX/sezero_tmp
		mv $PREFIX/sezero_tmp/$MINGW_BDIR/* $PREFIX
		rm -rf $PREFIX/sezero_tmp
		# Update headers and libs
		echo "Install update pack..."
		unzip -q -o $DOWNLOADS/$UPDATE_PACK -d $PREFIX
		# Copy over all DDK headers
		echo "Copy over DDK headers..."
		cp -a $DDK_MIRROR/include $PREFIX/$TARGET
		rm -rf $PREFIX/$TARGET/include/.svn $PREFIX/$TARGET/include/ddk/.svn
	fi
}


if [ "$COLINUX_HOST_ARCH"="x86_64" ]
then
	echo -n "Check cross compiler: "
	cd "$TOPDIR"
	if [ -x $PREFIX/bin/$TARGET-gcc -a \
	     -x $PREFIX/bin/$TARGET-ld -a \
	     -x $PREFIX/bin/$TARGET-windres -a \
	     -x $PREFIX/bin/$TARGET-strip ]
	then
		echo "Skip $TARGET-gcc, $TARGET-ld"
		echo " - already installed on $PREFIX/bin"
		exit 0
	fi

	settings64
	download64
	unpack64

	if [ -x $PREFIX/bin/$TARGET-gcc -a -f $PREFIX/$TARGET/include/ddk/ntddk.h ]
	then
		echo "MinGW-w64 installation done"
		exit 0
	else
		echo "MinGW-w64 installation failed"
		exit 1
	fi
fi

# Flags for building gcc (not for target)
BUILD_FLAGS="CFLAGS=-O2 LDFLAGS=-s"

# until release, disable checking for faster builds:
DISABLE_CHECKING=--disable-checking

download_files()
{
	download_file "$GCC_ARCHIVE1" "$GCC_URL"
	download_file "$GCC_ARCHIVE2" "$GCC_URL"
	download_file "$BINUTILS_ARCHIVE" "$BINUTILS_URL"
	download_file "$MINGW_ARCHIVE" "$MINGW_URL"
	download_file "$W32API_ARCHIVE" "$MINGW_URL"
}

check_installed()
{
	echo -n "Check cross compiler: "
	cd "$TOPDIR"
	if [ -x $PREFIX/bin/$TARGET-gcc -a \
	     -x $PREFIX/bin/$TARGET-ld -a \
	     -x $PREFIX/bin/$TARGET-windres -a \
	     -x $PREFIX/bin/$TARGET-strip ]
	then
		# Verify version of installed GCC and LD
		if [ `$TARGET-gcc -dumpversion` != $GCC_VERSION ]
		then
			echo "$TARGET-gcc $GCC_VERSION not installed"
			return
		fi

		if $TARGET-ld --version | egrep -q "$BINUTILS_VERSION"
		then
			echo "Skip $TARGET-gcc, $TARGET-ld"
			echo " - already installed on $PREFIX/bin"
			exit 0
		else
			echo "$TARGET-ld $BINUTILS_VERSION not installed"
			return
		fi

	fi
	echo "No executable, rebuilding"
}


install_libs()
{
	echo "Installing cross libs and includes"
	tar_unpack_to $MINGW_ARCHIVE "$PREFIX/$TARGET"
	tar_unpack_to $W32API_ARCHIVE "$PREFIX/$TARGET"
}

extract_binutils()
{
	echo "Extracting binutils"
	rm -rf "$BUILD_DIR/$BINUTILS"
	tar_unpack_to "$BINUTILS_ARCHIVE" "$BUILD_DIR"
}

patch_binutils()
{
	# Fixup wrong path in tar
	test -d $BUILD_DIR/${BINUTILS}-src && mv $BUILD_DIR/${BINUTILS}-src $BUILD_DIR/${BINUTILS}
	if [ "$BINUTILS_PATCH" != "" ]; then
		echo "Patching binutils"
		cd "$BUILD_DIR/$BINUTILS"
		patch -p1 < "$TOPDIR/$BINUTILS_PATCH"
		test $? -ne 0 && error_exit 10 "patch binutils failed"
	fi
}

configure_binutils()
{
	echo "Configuring binutils"
	cd "$BUILD_DIR"
	rm -rf "binutils-$TARGET"
	mkdir "binutils-$TARGET"
	cd "binutils-$TARGET"
	"../$BINUTILS/configure" \
		--prefix="$PREFIX" --target=$TARGET \
		--disable-nls \
		>>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "configure binutils failed"
}

build_binutils()
{
	echo "Building binutils"
	cd "$BUILD_DIR/binutils-$TARGET"
	make $BUILD_FLAGS >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "make binutils failed"
}

install_binutils()
{
	echo "Installing binutils"
	cd "$BUILD_DIR/binutils-$TARGET"
	make install >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "install binutils failed"
}

clean_binutils()
{
	echo "Clean binutils"
	cd $BUILD_DIR
	rm -rf "$BINUTILS" binutils-$TARGET binutils-$COLINUX_GCC_GUEST_TARGET
}

extract_gcc()
{
	echo "Extracting gcc"
	rm -rf "$BUILD_DIR/$GCC"
	tar_unpack_to "$GCC_ARCHIVE1" "$BUILD_DIR"
	tar_unpack_to "$GCC_ARCHIVE2" "$BUILD_DIR"
}

patch_gcc()
{
	if [ "$GCC_PATCH" != "" ]; then
		echo "Patching gcc"
		cd "$BUILD_DIR/$GCC"
		patch -p1 < "$TOPDIR/$GCC_PATCH"
		test $? -ne 0 && error_exit 10 "patch gcc failed"
	fi
}

configure_gcc()
{
	echo "Configuring gcc"
	cd "$BUILD_DIR"
	rm -rf "gcc-$TARGET"
	mkdir "gcc-$TARGET"
	cd "gcc-$TARGET"
	"../$GCC/configure" -v \
		--prefix="$PREFIX" --target=$TARGET \
		--with-headers="$PREFIX/$TARGET/include" \
		--with-gnu-as --with-gnu-ld \
		--disable-nls \
		--without-newlib --disable-multilib \
		--enable-languages="c,c++" \
		$DISABLE_CHECKING \
		>>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "configure gcc failed"
}

build_gcc()
{
	echo "Building gcc"
	cd "$BUILD_DIR/gcc-$TARGET"
	make $BUILD_FLAGS >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "make gcc failed"
}

install_gcc()
{
	echo "Installing gcc"
	cd "$BUILD_DIR/gcc-$TARGET"
	make install >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "install gcc failed"
}

clean_gcc()
{
	echo "Clean gcc"
	cd $BUILD_DIR
	rm -rf "$GCC" gcc-$TARGET gcc-$COLINUX_GCC_GUEST_TARGET
}

#
# Build compiler for coLinux GUEST system kernel compiling.
#

check_binutils_guest()
{
	test -z "$COLINUX_GCC_GUEST_TARGET" && return 0
	echo -n "Check guest binutils $BINUTILS_VERSION: "

	# Get version number
	local PATH="$PATH:$COLINUX_GCC_GUEST_PATH"
	ver=`${COLINUX_GCC_GUEST_TARGET}-as --version 2>/dev/null || \
		as --version 2>/dev/null | \
		sed -n -e 's/^[^0-9]*\([0-9]\{1,\}\.[0-9]]\{1,\}\.[0-9]]\{1,\}\).*$/\1/p'`

	if [ -z "$ver" ]
	then
		echo "No executables, build now"
		return 1
	fi

	# Verify version of installed AS
	if [ $ver != $BINUTILS_VERSION ]
	then
		echo "Wrong version ($ver), build now"
		return 1
	fi

	echo "found"

	# Must have prefix for kernel build
	for name in ar as ld nm objdump objcopy strip
	do
		if ! which ${COLINUX_GCC_GUEST_TARGET}-$name >/dev/null 2>/dev/null
		then
			mkdir -p $COLINUX_GCC_GUEST_PATH
			ln -s `which $name` $COLINUX_GCC_GUEST_PATH/${COLINUX_GCC_GUEST_TARGET}-$name
			echo " softlink for ${COLINUX_GCC_GUEST_TARGET}-$name"
		fi
	done

	return 0
}

build_binutils_guest()
{
	echo "Configuring guest binutils"
	cd "$BUILD_DIR"
	rm -rf "binutils-$COLINUX_GCC_GUEST_TARGET"
	mkdir "binutils-$COLINUX_GCC_GUEST_TARGET"
	cd "binutils-$COLINUX_GCC_GUEST_TARGET"
	"../$BINUTILS/configure" \
		--program-prefix="${COLINUX_GCC_GUEST_TARGET}-" \
		--prefix="$PREFIX/$COLINUX_GCC_GUEST_TARGET" \
		--disable-nls \
		>>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "configure guest binutils failed"

	echo "Building guest binutils"
	make $BUILD_FLAGS >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "make guest binutils failed"

	echo "Installing guest binutils"
	make install >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "install guest binutils failed"
}

check_gcc_guest()
{
	test -z "$COLINUX_GCC_GUEST_TARGET" && return 0
	echo -n "Check guest compiler $GCC_VERSION: "

	# Get version number
	local PATH="$PATH:$COLINUX_GCC_GUEST_PATH"
	ver=`${COLINUX_GCC_GUEST_TARGET}-gcc -dumpversion 2>/dev/null`

	if [ -z "$ver" ]
	then
		echo "No executables, build now"
		return 1
	fi

	# Verify version of installed GCC
	if [ $ver != $GCC_VERSION ]
	then
		echo "Wrong version ($ver), build now"
		return 1
	fi

	echo "found"
	return 0
}

build_gcc_guest()
{
	echo "Configuring guest gcc"
	cd "$BUILD_DIR"
	rm -rf "gcc-$COLINUX_GCC_GUEST_TARGET"
	mkdir "gcc-$COLINUX_GCC_GUEST_TARGET"
	cd "gcc-$COLINUX_GCC_GUEST_TARGET"
	"../$GCC/configure" -v \
		--program-prefix="${COLINUX_GCC_GUEST_TARGET}-" \
		--prefix="$PREFIX/$COLINUX_GCC_GUEST_TARGET" \
		--disable-nls \
		--enable-languages="c,c++" \
		$DISABLE_CHECKING \
		>>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "configure gcc failed"

	echo "Building guest gcc"
	make $BUILD_FLAGS >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "make guest gcc failed"

	echo "Installing guest gcc"
	make install >>$COLINUX_BUILD_LOG 2>>$COLINUX_BUILD_ERR
	test $? -ne 0 && error_exit 1 "install guest gcc failed"

	# remove info and man pages
	rm -rf "$PREFIX/$COLINUX_GCC_GUEST_TARGET/info" "$PREFIX/$COLINUX_GCC_GUEST_TARGET/man"
}

final_tweaks()
{
	echo "Finalizing installation"

	# remove gcc build headers
	rm -rf "$PREFIX/$TARGET/sys-include"

	# remove info and man pages
	rm -rf "$PREFIX/info" "$PREFIX/man"

	echo "Installation complete!"
}

build_cross()
{
	# do not check files, if rebuild forced
	test "$1" = "--rebuild" || check_installed

	download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0

	echo "log: $COLINUX_BUILD_LOG"
	mkdir -p `dirname $COLINUX_BUILD_LOG`
	echo "err: $COLINUX_BUILD_ERR"
	mkdir -p `dirname $COLINUX_BUILD_ERR`

	install_libs

	extract_binutils
	patch_binutils
	configure_binutils
	build_binutils
	install_binutils
	check_binutils_guest || build_binutils_guest
	clean_binutils

	extract_gcc
	patch_gcc
	configure_gcc
	build_gcc
	install_gcc
	check_gcc_guest || build_gcc_guest
	clean_gcc

	final_tweaks
}

build_cross $1
