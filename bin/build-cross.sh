#!/bin/sh

source ./build-common.sh

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$GCC_ARCHIVE1" "$MINGW_URL"
	download_file "$GCC_ARCHIVE2" "$MINGW_URL"
	download_file "$BINUTILS_ARCHIVE" "$MINGW_URL"
	download_file "$MINGW_ARCHIVE" "$MINGW_URL"
	download_file "$W32API_ARCHIVE" "$MINGW_URL"
}

check_md5sums()
{
	echo "Check md5sum"
	if md5sum -c $BINUTILS_CHECKSUM >>$COLINUX_BUILD_LOG 2>&1 ; then
		echo "Skip $TARGET-gcc, $TARGET-ld, $TARGET-windres"
		echo " - already installed on $PREFIX/bin"
		exit 0
	fi
}

create_md5sums()
{
	echo "Create md5sum"
	md5sum -b \
	    $GCC_PATCH \
	    $PREFIX/bin/$TARGET-windres \
	    $PREFIX/bin/$TARGET-ld \
	    $PREFIX/bin/$TARGET-gcc \
	    $PREFIX/$TARGET/bin/strip \
	    > $BINUTILS_CHECKSUM
	test $? -ne 0 && error_exit 1 "can not create md5sum"
	if [ "$GCC_PATCH" != "" ]; then
		md5sum -b $SRCDIR/$GCC_PATCH >> $BINUTILS_CHECKSUM
	fi
}


install_libs()
{
	echo "Installing cross libs and includes"
	mkdir -p "$PREFIX/$TARGET"
	cd "$PREFIX/$TARGET"
	gzip -dc "$SRCDIR/$MINGW_ARCHIVE" | tar x
	gzip -dc "$SRCDIR/$W32API_ARCHIVE" | tar x
	cd "$BINDIR"
}

extract_binutils()
{
	cd "$SRCDIR"
	rm -rf "$BINUTILS"
	echo "Extracting binutils"
	gzip -dc "$SRCDIR/$BINUTILS_ARCHIVE" | tar x
	cd "$BINDIR"
}

configure_binutils()
{
	cd "$BINDIR"
	rm -rf "binutils-$TARGET"
	mkdir "binutils-$TARGET"
	cd "binutils-$TARGET"
	echo "Configuring binutils"
	"$SRCDIR/$BINUTILS/configure" --prefix="$PREFIX" --target=$TARGET &> configure.log
	cd "$BINDIR"
}

build_binutils()
{
	cd "$BINDIR/binutils-$TARGET"
	echo "Building binutils"
	make >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "make binutils failed"
	cd "$BINDIR"
}

install_binutils()
{
	cd "$BINDIR/binutils-$TARGET"
	echo "Installing binutils"
	make install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "install binutils failed"
	cd "$BINDIR"
}

extract_gcc()
{
	cd "$SRCDIR"
	rm -rf "$GCC"
	echo "Extracting gcc"
	gzip -dc "$SRCDIR/$GCC_ARCHIVE1" | tar x
	gzip -dc "$SRCDIR/$GCC_ARCHIVE2" | tar x
	cd "$BINDIR"
}

patch_gcc()
{
	if [ "$GCC_PATCH" != "" ]; then
		echo "Patching gcc"
		cd "$SRCDIR/$GCC"
		patch -p1 < "$SRCDIR/$GCC_PATCH"
		cd "$BINDIR"
	fi
}

configure_gcc()
{
	cd "$BINDIR"
	rm -rf "gcc-$TARGET"
	mkdir "gcc-$TARGET"
	cd "gcc-$TARGET"
	echo "Configuring gcc"
	"$SRCDIR/$GCC/configure" -v \
		--prefix="$PREFIX" --target=$TARGET \
		--with-headers="$PREFIX/$TARGET/include" \
		--with-gnu-as --with-gnu-ld \
		--without-newlib --disable-multilib >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "configure gcc failed"
	cd "$BINDIR"
}

build_gcc()
{
	cd "$BINDIR/gcc-$TARGET"
	echo "Building gcc"
	make LANGUAGES="c c++" >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "make gcc failed"
	cd "$BINDIR"
}

install_gcc()
{
	cd "$BINDIR/gcc-$TARGET"
	echo "Installing gcc"
	make LANGUAGES="c c++" install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "install gcc failed"
	cd "$BINDIR"
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
	ls "$PREFIX"/bin/* "$PREFIX/$TARGET"/bin/* | egrep -v '.dll$' |
	while read file; do
		strip "$file"
	done
	
	# Installation should have been successful, so clean-up
	#  after ourselves an little bit.
	rm -rf *i686-pc-mingw32

	echo "Installation complete!"
}

build_cross()
{
        download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0

	# do not check files, if rebuild forced
	test "$1" = "--rebuild" || check_md5sums

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
	create_md5sums
}

build_cross $1
