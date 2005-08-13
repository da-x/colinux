#!/bin/sh

# Build cross platform mingw32.

. build-common.sh

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$GCC_ARCHIVE1" "$MINGW_URL"
	download_file "$GCC_ARCHIVE2" "$MINGW_URL"
	download_file "$BINUTILS_ARCHIVE" "$MINGW_URL"
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
			return 1
		fi

		if $TARGET-ld --version | egrep -q "$BINUTILS_VERSION"
		then
			echo "Skip $TARGET-gcc, $TARGET-ld"
			echo " - already installed on $PREFIX/bin"
			exit 0
		else
			echo "$TARGET-ld $BINUTILS_VERSION not installed"
			return 1
		fi

	fi
	echo "No executable, rebuilding"
}


install_libs()
{
	echo "Installing cross libs and includes"
	mkdir -p "$PREFIX/$TARGET"
	cd "$PREFIX/$TARGET"
	gzip -dc "$SRCDIR/$MINGW_ARCHIVE" | tar x
	gzip -dc "$SRCDIR/$W32API_ARCHIVE" | tar x
}

extract_binutils()
{
	echo "Extracting binutils"
	mkdir -p "$BUILD_DIR"
	cd "$BUILD_DIR"
	rm -rf "$BINUTILS"
	gzip -dc "$SRCDIR/$BINUTILS_ARCHIVE" | tar x
}

configure_binutils()
{
	echo "Configuring binutils"
	cd "$BUILD_DIR"
	rm -rf "binutils-$TARGET"
	mkdir "binutils-$TARGET"
	cd "binutils-$TARGET"
	"$BUILD_DIR/$BINUTILS/configure" \
	 --prefix="$PREFIX" --target=$TARGET >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "configure binutils failed"
}

build_binutils()
{
	echo "Building binutils"
	cd "$BUILD_DIR/binutils-$TARGET"
	make >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "make binutils failed"
}

install_binutils()
{
	echo "Installing binutils"
	cd "$BUILD_DIR/binutils-$TARGET"
	make install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "install binutils failed"
}

extract_gcc()
{
	echo "Extracting gcc"
	cd "$BUILD_DIR"
	rm -rf "$GCC"
	gzip -dc "$SRCDIR/$GCC_ARCHIVE1" | tar x
	gzip -dc "$SRCDIR/$GCC_ARCHIVE2" | tar x
}

patch_gcc()
{
	if [ "$GCC_PATCH" != "" ]; then
		echo "Patching gcc"
		cd "$BUILD_DIR/$GCC"
		patch -p1 < "$SRCDIR/$GCC_PATCH"
	fi
}

configure_gcc()
{
	echo "Configuring gcc"
	cd "$BUILD_DIR"
	rm -rf "gcc-$TARGET"
	mkdir "gcc-$TARGET"
	cd "gcc-$TARGET"
	"$BUILD_DIR/$GCC/configure" -v \
		--prefix="$PREFIX" --target=$TARGET \
		--with-headers="$PREFIX/$TARGET/include" \
		--with-gnu-as --with-gnu-ld \
		--without-newlib --disable-multilib >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "configure gcc failed"
}

build_gcc()
{
	echo "Building gcc"
	cd "$BUILD_DIR/gcc-$TARGET"
	make LANGUAGES="c c++" >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "make gcc failed"
}

install_gcc()
{
	echo "Installing gcc"
	cd "$BUILD_DIR/gcc-$TARGET"
	make LANGUAGES="c c++" install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "install gcc failed"
}

final_tweaks()
{
	echo "Finalizing installation"

	# remove gcc build headers
	rm -rf "$PREFIX/$TARGET/sys-include"

        # Add extra binary links
	if [ ! -f "$PREFIX/$TARGET/bin/objdump" ]; then
		ln "$PREFIX/bin/$TARGET-objdump" "$PREFIX/$TARGET/bin/objdump"
	fi

	# make cc and c++ symlinks to gcc and g++
	if [ ! -f "$PREFIX/$TARGET/bin/g++" ]; then
		ln "$PREFIX/bin/$TARGET-g++" "$PREFIX/$TARGET/bin/g++"
	fi
	if [ ! -f "$PREFIX/$TARGET/bin/cc" ]; then
		ln -s "gcc" "$PREFIX/$TARGET/bin/cc"
	fi
	if [ ! -f "$PREFIX/$TARGET/bin/c++" ]; then
		ln -s "g++" "$PREFIX/$TARGET/bin/c++"
	fi

	# strip all the binaries
	ls $PREFIX/bin/* $PREFIX/$TARGET/bin/* | egrep -v '.dll|gccbug' |
	while read file; do
		strip "$file"
	done
	
	# Installation should have been successful, so clean-up
	#  after ourselves an little bit.
	cd $BUILD_DIR
	rm -rf *i686-pc-mingw32 "$BINUTILS" "$GCC"

	echo "Installation complete!"
}

build_cross()
{
	# do not check files, if rebuild forced
	test "$1" = "--rebuild" || check_installed

	download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0

        install_libs

        extract_binutils
        configure_binutils
        build_binutils
        install_binutils

        extract_gcc
        patch_gcc
        configure_gcc
        build_gcc
        install_gcc

        final_tweaks
}

build_cross $1
