#!/bin/sh

. ./build-common.sh

PATH="$PREFIX/$TARGET/bin:$PATH"

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$FLTK_ARCHIVE" "$FLTK_URL"
	download_file "$MXML_ARCHIVE" "$MXML_URL"
	download_file "$W32API_SRC_ARCHIVE" "$MINGW_URL"
	download_file "$WINPCAP_SRC_ARCHIVE" "$WINPCAP_URL"
}

check_md5sums()
{
	echo "Check md5sum"
	cd "$TOPDIR/.."
	if md5sum -c $W32LIBS_CHECKSUM >>$COLINUX_BUILD_LOG 2>&1 ; then
		echo "Skip w32api.h, libfltk.a, libmxml.a, libwin32k.a"
		echo " - already installed on $PREFIX/$TARGET/lib"
		exit 0
	fi
	cd "$TOPDIR"
}

create_md5sums()
{
	echo "Create md5sum"
	cd "$TOPDIR/.."
	md5sum -b \
	    patch/$FLTK-win32.diff \
	    $W32API_PATCH \
	    $PREFIX/$TARGET/include/w32api.h \
	    $PREFIX/$TARGET/lib/libfltk.a \
	    $PREFIX/$TARGET/lib/libmxml.a \
	    $PREFIX/$TARGET/lib/libwin32k.a \
	    $PREFIX/$TARGET/include/.fltk.version \
	    $PREFIX/$TARGET/include/.mxml.version \
	    $PREFIX/$TARGET/include/.w32api.version \
	    > $W32LIBS_CHECKSUM
	test $? -ne 0 && error_exit 1 "can not create md5sum"
	cd "$TOPDIR"
}

#
# FLTK
#

extract_fltk()
{
	cd "$SRCDIR"
	rm -rf "$FLTK"
	echo "Extracting FLTK"
	bzip2 -dc "$SRCDIR/$FLTK_ARCHIVE" | tar x
	cd "$TOPDIR"
}

patch_fltk()
{
	cd "$SRCDIR/$FLTK"
	patch -p1 < "$TOPDIR/../patch/$FLTK-win32.diff"
	test $? -ne 0 && error_exit 1 "FLTK patch failed"
	cd "$TOPDIR"
}

configure_fltk()
{
	cd "$SRCDIR/$FLTK"
	echo "Configuring FLTK"

	# X11 is installed for Target? Use, if exist. (Fake for X11-less compiling)
	if [ -f $PREFIX/include/X11/X.h -a -f $PREFIX/lib/X11/libX11.a ]; then
	    prefix_x11="--x-includes=$PREFIX/include --x-libraries=$PREFIX/lib"
	fi

	# Using of --host=$TARGET ist old!
	# Plesae beleve host=i386, also your host is mostly i686!
	# "i386..." is a pseudonym to enable "cross-compiling",
	# because target is diffent with "i686..."
	# Configure for cross compiling. Host Linux
	./configure \
	 --prefix=$PREFIX \
	 --build=$TARGET \
	 $prefix_x11 \
	 --host=i386-linux-linux-gnu
	test $? -ne 0 && error_exit 1 "FLTK configure failed"
	echo "Making FLTK"
	make -C src >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "FLTK make failed"
	cd "$TOPDIR"
}

install_fltk()
{
	cd "$SRCDIR/$FLTK"
	echo "Installing FLTK"
	cp lib/*.a $PREFIX/$TARGET/lib
	cp -a FL $PREFIX/$TARGET/include/
	echo "$FLTK_VERSION" >$PREFIX/$TARGET/include/.fltk.version
	cd "$TOPDIR"
}

build_fltk()
{
	extract_fltk
	patch_fltk
	configure_fltk
	install_fltk
}

#
# MXML
#

extract_mxml()
{
	cd "$SRCDIR"
	rm -rf "$MXML"
	echo "Extracting MXML"
	gzip -dc "$SRCDIR/$MXML_ARCHIVE" | tar x
	cd "$TOPDIR"
}

configure_mxml()
{
	cd "$SRCDIR/$MXML"
	echo "Configuring MXML"
	./configure --host=$TARGET >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "MXML configure failed"
	echo "Making MXML"
	make libmxml.a >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "MXML make failed"
	cd "$TOPDIR"
}

install_mxml()
{
	cd "$SRCDIR/$MXML"
	echo "Installing MXML"
	cp libmxml.a $PREFIX/$TARGET/lib
	cp mxml.h $PREFIX/$TARGET/include
	echo "$MXML_VERSION" >$PREFIX/$TARGET/include/.mxml.version
	cd "$TOPDIR"
}

build_mxml()
{
	extract_mxml
	configure_mxml
	install_mxml
}

#
# w32api_src source
#

extract_w32api_src()
{
	echo "Extracting w32api source"
	cd "$SRCDIR"
	rm -rf "$W32API_SRC"
	gzip -dc "$SRCDIR/$W32API_SRC_ARCHIVE" | tar x
	cd "$TOPDIR"
}

patch_w32api_src()
{
	if [ -z "$W32API_PATCH" ]; then
		echo "Patching w32api - $TOPDIR/../$W32API_PATCH"
		cd "$SRCDIR/$W32API_SRC"
		patch -p1 < "$TOPDIR/../$W32API_PATCH"
		test $? -ne 0 && error_exit 1 "w32api source patch failed"
		cd "$TOPDIR"
	fi
}

configure_w32api_src()
{
	cd "$SRCDIR/$W32API_SRC"
	echo "Configuring w32api source"
	./configure --host=$TARGET --prefix=$PREFIX/$TARGET \
		CC=$TARGET-gcc >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "w32api source configure failed"
	echo "Making w32api source"
	make >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "w32api source make failed"
	cd "$TOPDIR"
}

install_w32api_src()
{
	cd "$SRCDIR/$W32API_SRC"
	echo "Installing $W32API_SRC"
	make install >>$COLINUX_BUILD_LOG 2>&1
	test $? -ne 0 && error_exit 1 "w32api make install failed"
	echo "$W32API_VERSION" >$PREFIX/$TARGET/include/.w32api.version
	cd "$TOPDIR"
}

build_w32api_src()
{
	extract_w32api_src
	patch_w32api_src
	configure_w32api_src
	install_w32api_src
}

# WinPCAP

extract_winpcap_src()
{
	cd "$SRCDIR"
	rm -rf "$WINPCAP_SRC"
	echo "Extracting winpcap source"
	unzip "$WINPCAP_SRC_ARCHIVE" >>$COLINUX_BUILD_LOG 2>&1
	cd "$TOPDIR"
}

install_winpcap_src()
{
	cd "$SRCDIR/$WINPCAP_SRC"
	echo "Installing $WINPCAP_SRC"
	cp Include/PCAP.H "$PREFIX/$TARGET/include/pcap.h"
	cp Include/pcap-stdinc.h "$PREFIX/$TARGET/include"
	cp Include/bittypes.h "$PREFIX/$TARGET/include"
	cp Include/ip6_misc.h "$PREFIX/$TARGET/include"
	if ! test -d "$PREFIX/$TARGET/include/net"; then
		mkdir "$PREFIX/$TARGET/include/net"
	fi
	cp Include/NET/*.h "$PREFIX/$TARGET/include/net/"
	cp Lib/libwpcap.a "$PREFIX/$TARGET/lib"
	if test $? -ne 0; then
	        echo "winpcap install failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

build_winpcap_src()
{
	extract_winpcap_src
	install_winpcap_src
}

# ALL

build_colinux_libs()
{
        download_files
	# Only Download? Than ready.
	test "$1" = "--download-only" && exit 0

	# do not check files, if rebuild forced
	test "$1" = "--rebuild" || check_md5sums

	build_fltk
	build_mxml
	build_w32api_src
	build_winpcap_src

	create_md5sums
}

build_colinux_libs $1
