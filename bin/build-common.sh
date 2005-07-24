#!/bin/sh

#
# This script builds the mingw32 cross compiler on Linux,
# and patches the w32api package.
#
# It also downloads, compiles, and installs FLTK and mxml.
# 
# - Dan Aloni <da-x@colinux.org>
#
# This file will source from toplevel Makefile.
# For backwards, can source from command line for sample:
#  . ./build-common.sh --build-all
#
# Options MUST ones of (and only ones):
#  --build-all		Build all, without checking old targed files.
#			(Old file was build-all.sh)
#  --rebuild-all	Rebuild all, without checking old targed files.
#			Disable md5sum, untar and patch source.
#			Overwrite all old source!
#  --download-all	Download all source files, no compile
#			(Old file was download-all.sh)


#####################################################################
# This is my script for building a complete cross-compiler toolchain.
# It is based partly on Ray Kelm's script, which in turn was built on
# Mo Dejong's script for doing the same, but with some added fixes.
# The intent with this script is to build a cross-compiled version
# of the current MinGW environment.
#
# Updated by Sam Lantinga <slouken@libsdl.org>
#####################################################################

# Use User config, if exist
if [ -f user-build.cfg ] ; then
	# Users directories
	. ./user-build.cfg
else
	# fall back to default config
	. ./sample.user-build.cfg
fi

# what flavor are we building?
TARGET=i686-pc-mingw32

# you probably don't need to change anything from here down
TOPDIR=`pwd`
SRCDIR="$SOURCE_DIR"

# (from build-cross.sh) #

# Updated by Sam Lantinga <slouken@libsdl.org>
# These are the files from the current MingW release

MINGW_VERSION="3.7"
MINGW_URL=http://heanet.dl.sourceforge.net/sourceforge/mingw
MINGW=mingw-runtime-$MINGW_VERSION
MINGW_ARCHIVE=$MINGW.tar.gz

BINUTILS_VERSION="2.15.91"
BINUTILS_RELEASE="$BINUTILS_VERSION-20040904-1"
BINUTILS=binutils-$BINUTILS_RELEASE
BINUTILS_ARCHIVE=$BINUTILS-src.tar.gz
BINUTILS_CHECKSUM=$SRCDIR/.build-cross.md5

GCC_VERSION="3.3.1"
GCC_RELEASE="$GCC_VERSION-20030804-1"
GCC=gcc-$GCC_RELEASE
GCC_ARCHIVE1=gcc-core-$GCC_RELEASE-src.tar.gz
GCC_ARCHIVE2=gcc-g++-$GCC_RELEASE-src.tar.gz
GCC_PATCH=""

W32API_VERSION=3.2
W32API=w32api-$W32API_VERSION
W32API_SRC=$W32API
W32API_SRC_ARCHIVE=$W32API-src.tar.gz
W32API_ARCHIVE=$W32API.tar.gz
# Patch can be empty or comment out, if not need
W32API_PATCH=patch/$W32API_SRC.diff
W32LIBS_CHECKSUM=$SRCDIR/.build-colinux-libs.md5


# (from build-colinux-libs.sh) #
FLTK_VERSION="1.1.4"
FLTK_URL=http://heanet.dl.sourceforge.net/sourceforge/fltk 
FLTK=fltk-$FLTK_VERSION
FLTK_ARCHIVE=$FLTK-source.tar.bz2

MXML_VERSION="1.3"
MXML_URL=http://www.easysw.com/~mike/mxml/swfiles
# a mirror http://gniarf.nerim.net/colinux
MXML=mxml-$MXML_VERSION
MXML_ARCHIVE=$MXML.tar.gz

WINPCAP_SRC=WpdPack
WINPCAP_URL=http://winpcap.polito.it/install/bin
WINPCAP_SRC_ARCHIVE=${WINPCAP_SRC}_3_1_beta4.zip


# (from sample.user-build.cfg) #

# Kernel version we are targeting
# Remember: Please update also conf/kernel-config, if changing kernel version!
#
KERNEL_DIR="v2.6"
KERNEL_VERSION="2.6.11"


# (from build-kernel.sh) #

KERNEL=linux-$KERNEL_VERSION
KERNEL_URL=http://www.kernel.org/pub/linux/kernel/$KERNEL_DIR
KERNEL_ARCHIVE=$KERNEL.tar.bz2
KERNEL_CHECKSUM=$SRCDIR/.build-kernel.md5


# where does it go?
if [ "$PREFIX" = "" ] ; then
    echo "Please specify the $""PREFIX directory in user-build.cfg (e.g, /usr/local/mingw32)"
    exit -1
fi

# where does it go?
if [ "$SOURCE_DIR" = "" ] ; then
    echo "Please specify the $""SOURCE_DIR directory in user-build.cfg (e.g, /tmp/$USER/download)"
    exit -1
fi

# coLinux enabled kernel source?
if [ "$COLINUX_TARGET_KERNEL_PATH" = "" ] ; then
    echo "Please specify the $""COLINUX_TARGET_KERNEL_PATH in user-build.cfg (e.g, /tmp/$USER/linux-co)"
    exit -1
fi

# coLinux kernel we are targeting
# KERNEL_DIR: sub-dir in www.kernel.org for the download (e.g. v2.6)
if [ "$KERNEL_DIR" = "" ] ; then
    echo "Please specify the $""KERNEL_DIR in user-build.cfg (e.g. KERNEL_DIR=v2.6"
    exit -1
fi
# KERNEL_VERSION: the full kernel version (e.g. 2.6.10)
if [ "$KERNEL_VERSION" = "" ] ; then
    echo "Please specify the $""KERNEL_VERSION in user-build.cfg (e.g. KERNEL_VERSION=2.6.10"
    exit -1
fi

# Default logfile of building (Append), can overwrite in user-build.cfg
if [ "$COLINUX_BUILD_LOG" = "" ] ; then
    COLINUX_BUILD_LOG="$TOPDIR/build-colinux-$$.log"
fi

# Default install directory
if [ "$COLINUX_INSTALL_DIR" = "" ] ; then
    echo "Please specify the $""COLINUX_INSTALL_DIR in user-build.cfg (e.g, /home/$USER/colinux/dist)"
    exit -1
fi

# These are the files from the SDL website
# need install directory first on the path so gcc can find binutils

PATH="$PREFIX/bin:$PATH"

#
# download a file from a given url, only if it is not present
#

download_file()
{
	# Make sure wget is installed
	if test "x`which wget`" = "x" ; then
		echo "You need to install wget."
		exit 1
	fi
	cd "$SRCDIR"
	if test ! -f $1 ; then
		echo "Downloading $1"
		wget "$2/$1"
		if test ! -f $1 ; then
			echo "Could not download $1"
			exit 1
		fi
	else
		echo "Found $1 in the srcdir $SRCDIR"
	fi
  	cd "$TOPDIR"
}

#
# Show errors from actual logfile, than exit build process
# Arg1: Errorlevel
# Arg2: Error message
#
error_exit()
{
	# Show errors in log file with tail or less
	tail -n 20 $COLINUX_BUILD_LOG
	# less $COLINUX_BUILD_LOG

	echo "$2"
	echo "  - log available: $COLINUX_BUILD_LOG"
	exit $1
}

build_all()
{
	./build-cross.sh $1 && \
	./build-colinux-libs.sh $1 && \
	./build-kernel.sh $1 && \
	./build-colinux.sh $1 && \
	echo "Build-all $1 DONE"
}

case "$1" in
    --build-all)
	build_all
	;;
    --download-all)
	build_all --download-only
	;;
    --rebuild-all)
	build_all --rebuild
	;;
    --help)
	echo "
	file: bin/build-common.sh

	Options: --build-all | --download-all | --rebuild-all | --help

	Whithout arguments is used for common set of variables and functions.
	For more information, read top of file."
	;;
esac
