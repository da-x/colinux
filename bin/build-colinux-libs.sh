#!/bin/sh

source ./build-common.sh

# See ./commom.sh

FLTK=fltk-1.1.4
FLTK_URL=http://heanet.dl.sourceforge.net/sourceforge/fltk 
FLTK_ARCHIVE=$FLTK-source.tar.bz2

MXML=mxml-1.3
MXML_URL=http://www.easysw.com/~mike/mxml/swfiles
# a mirror http://gniarf.nerim.net/colinux
MXML_ARCHIVE=$MXML.tar.gz

W32API_SRC=$W32API
W32API_SRC_ARCHIVE=$W32API-src.tar.gz

WINPCAP_SRC=wpdpack
WINPCAP_URL=http://winpcap.polito.it/install/bin
WINPCAP_SRC_ARCHIVE="$WINPCAP_SRC"_3_0.zip

PATH="$PREFIX/$TARGET/bin:$PATH"

download_files()
{
	mkdir -p "$SRCDIR"
	
	download_file "$FLTK_ARCHIVE" "$FLTK_URL"
	download_file "$MXML_ARCHIVE" "$MXML_URL"
	download_file "$W32API_SRC_ARCHIVE" "$MINGW_URL"
	download_file "$WINPCAP_SRC_ARCHIVE" "$WINPCAP_URL"
}

check_isinstalled()
{
	if [ -f $PREFIX/$TARGET/lib/libfltk.a -a -f $PREFIX/$TARGET/lib/libmxml.a ] ; then
		echo "Skip libfltk.a,libmxml.a, already installed on $PREFIX/$TARGET/lib"
		exit 0
	fi
}

#
# FLTK
#

extract_fltk()
{
	cd "$SRCDIR"
	rm -rf "$FLTK"
	echo "Extracting FLTK"
	bzip2 -dc "$SRCDIR/$FLTK_ARCHIVE" | tar xf -
	cd "$TOPDIR"
}

patch_fltk()
{
	cd "$SRCDIR/$FLTK"
	patch -p1 < "$TOPDIR/../patch/$FLTK-win32.diff"
	if test $? -ne 0; then
	        echo "FLTK patch failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

configure_fltk()
{
	cd "$SRCDIR/$FLTK"
	echo "Configuring FLTK"
	./configure --host=i686-pc-mingw32 &> configure.log
	if test $? -ne 0; then
	        echo "FLTK configure failed"
	        exit 1
	fi
	echo "Making FLTK"
	make -C src &> make.log
	if test $? -ne 0; then
	        echo "FLTK make failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_fltk()
{
	cd "$SRCDIR/$FLTK"
	echo "Installing FLTK"
	cp lib/*.a $PREFIX/$TARGET/lib
	cp -a FL $PREFIX/$TARGET/include/
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
	gzip -dc "$SRCDIR/$MXML_ARCHIVE" | tar xf -
	cd "$TOPDIR"
}

configure_mxml()
{
	cd "$SRCDIR/$MXML"
	echo "Configuring MXML"
	./configure --host=i686-pc-mingw32 &> configure.log
	if test $? -ne 0; then
	        echo "MXML configure failed"
	        exit 1
	fi
	echo "Making MXML"
	make libmxml.a &> make.log
	if test $? -ne 0; then
	        echo "MXML make failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_mxml()
{
	cd "$SRCDIR/$MXML"
	echo "Installing MXML"
	cp libmxml.a $PREFIX/$TARGET/lib
	cp mxml.h $PREFIX/$TARGET/include
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
	cd "$SRCDIR"
	rm -rf "$W32API_SRC"
	echo "Extracting w32api source"
	gzip -dc "$SRCDIR/$W32API_SRC_ARCHIVE" | tar xf -
	cd "$TOPDIR"
}

patch_w32api_src()
{
	cd "$SRCDIR/$W32API_SRC"
	patch -p1 < "$TOPDIR/../patch/$W32API_SRC.diff"
	if test $? -ne 0; then
	        echo "w32api source patch failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

configure_w32api_src()
{
	cd "$SRCDIR/$W32API_SRC"
	echo "Configuring w32api source"
	./configure --host=i686-pc-mingw32 --prefix=$PREFIX/$TARGET &> configure.log
	if test $? -ne 0; then
	        echo "w32api source configure failed"
	        exit 1
	fi
	echo "Making w32api source"
	make &> make.log
	if test $? -ne 0; then
	        echo "w32api source make failed"
	        exit 1
	fi
	cd "$TOPDIR"
}

install_w32api_src()
{
	cd "$SRCDIR/$W32API_SRC"
	echo "Installing $W32API_SRC"
	make install &> configure.log
	if test $? -ne 0; then
	        echo "w32api make install failed"
	        exit 1
	fi
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
	unzip "$WINPCAP_SRC_ARCHIVE" 2>&1 > /dev/null
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
	check_isinstalled
	build_fltk
	build_mxml
	build_w32api_src
	build_winpcap_src
}

build_colinux_libs $1
