#!/bin/sh

#
# This script builds the mingw32 cross compiler on Linux,
# and patches the w32api package.
#
# It also downloads, compiles, and installs FLTK and mxml.
# 
# - Dan Aloni <da-x@colinux.org>
#
# This file will source from toplevel Makefile and scripts.
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
	. user-build.cfg
else
	# fall back to default config
	. sample.user-build.cfg
fi

# what flavor are we building?
TARGET=i686-pc-mingw32

# you probably don't need to change anything from here down
# BINDIR is bin directory with scripts
BINDIR=`pwd`

# TOPDIR is the directory with ./configure
TOPDIR=`dirname $BINDIR`

# Downloads store here
SRCDIR="$SOURCE_DIR"

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


FLTK_VERSION="1.1.6"
FLTK_URL=http://heanet.dl.sourceforge.net/sourceforge/fltk 
FLTK=fltk-$FLTK_VERSION
FLTK_ARCHIVE=$FLTK-source.tar.bz2

MXML_VERSION="1.3"
MXML_URL=http://www.easysw.com/~mike/mxml/swfiles
# a mirror http://gniarf.nerim.net/colinux
MXML=mxml-$MXML_VERSION
MXML_ARCHIVE=$MXML.tar.gz

WINPCAP_VERSION="3_1_beta4"
WINPCAP_URL=http://winpcap.polito.it/install/bin
WINPCAP_SRC=WpdPack
WINPCAP_SRC_ARCHIVE=${WINPCAP_SRC}_$WINPCAP_VERSION.zip


# Kernel version we are targeting
# Remember: Please update also conf/kernel-config, if changing kernel version!
# Read version from filename of patchfile patch/linux-2.6.11.diff
# KERNEL_VERSION: full kernel version (e.g. 2.6.11)
# KERNEL_DIR: sub-dir in www.kernel.org for the download (e.g. v2.6)
#
KERNEL_VERSION=`ls $TOPDIR/patch/linux-*.diff | sed -r -e 's/^.+\-([0-9\.]+)\.diff$/\1/'`
KERNEL_DIR=`echo $KERNEL_VERSION | sed -r -e 's/^([0-9]+)\.([0-9]+)\..+$/v\1.\2/'`

CO_VERSION=`cat $TOPDIR/src/colinux/VERSION`
COMPLETE_KERNEL_NAME=$KERNEL_VERSION-co-$CO_VERSION

KERNEL=linux-$KERNEL_VERSION
KERNEL_URL=http://www.kernel.org/pub/linux/kernel/$KERNEL_DIR
KERNEL_ARCHIVE=$KERNEL.tar.bz2
KERNEL_PATCH=patch/linux-$KERNEL_VERSION.diff

# Developer private patchfile. Used after maintainer patch,
# use also for manipulate ".config". Sourced in kernel build directory.
PRIVATE_PATCH=patch/linux-private.patch

# MD5sum files stored here
MD5DIR="$BUILD_DIR"
W32LIBS_CHECKSUM="$MD5DIR/.build-colinux-libs.md5"
KERNEL_CHECKSUM="$MD5DIR/.build-kernel.md5"

# coLinux kernel we are targeting
if [ -z "$KERNEL_VERSION" -o -z "$KERNEL_DIR" ] ; then
    # What's wrong here?
    echo "Can't find the kernel patch, probably wrong script,"
    echo "or file patch/linux-*.diff don't exist?"
    exit -1
fi

# Get variables for configure only? Than end here.
if [ "$1" = "--get-vars" ]; then
    return
fi

# where does it go?
if [ -z "$PREFIX" ] ; then
    echo "Please specify the $""PREFIX directory in user-build.cfg (e.g, /home/$USER/mingw32)"
    exit -1
fi

# where does it go?
if [ -z "$SOURCE_DIR" ] ; then
    echo "Please specify the $""SOURCE_DIR directory in user-build.cfg (e.g, /tmp/$USER/download)"
    exit -1
fi

# where does it go?
if [ -z "$BUILD_DIR" ] ; then
    echo "Please specify the $""BUILD_DIR directory in user-build.cfg (e.g, /tmp/$USER/build)"
    exit -1
fi

# coLinux enabled kernel source?
if [ -z "$COLINUX_TARGET_KERNEL_PATH" ] ; then
    echo "Please specify the $""COLINUX_TARGET_KERNEL_PATH in user-build.cfg (e.g, /tmp/$USER/linux-co)"
    exit -1
fi

# Default path to modules
if [ -z "$COLINUX_TARGET_MODULE_PATH" ] ; then
    COLINUX_TARGET_MODULE_PATH="$COLINUX_TARGET_KERNEL_PATH/_install"
fi

# Default logfile of building (Append), can overwrite in user-build.cfg
if [ -z "$COLINUX_BUILD_LOG" ] ; then
    COLINUX_BUILD_LOG="$TOPDIR/build-colinux-$$.log"
fi

# Install directory set?
if [ -z "$COLINUX_INSTALL_DIR" ] ; then
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
  	cd "$BINDIR"
}

#
# Show errors from actual logfile, than exit build process
# Arg1: Errorlevel
# Arg2: Error message
#
error_exit()
{
	# Show errors in log file with tail or less, only if errorlevel < 10
	if [ $1 -lt 10 ]; then
		echo -e "\n  --- ERROR LOG:"
		tail -n 20 $COLINUX_BUILD_LOG
		# less $COLINUX_BUILD_LOG
		echo "$2"
		echo "  --- log available: $COLINUX_BUILD_LOG"
	else
		echo "$2"
	fi

	exit $1
}

# Create ZIP packages (for "autobuild")
build_package()
{
	local name bname oname
	local STRIP="$TARGET-strip --strip-all"
	local SYMBOLS_ZIP=$COLINUX_INSTALL_DIR/symbols-$CO_VERSION.zip
	local DAEMONS_ZIP=$COLINUX_INSTALL_DIR/daemons-$CO_VERSION.zip
	local VMLINUX_ZIP=$COLINUX_INSTALL_DIR/vmlinux-$CO_VERSION.zip
	local MODULES_TGZ=$COLINUX_INSTALL_DIR/modules-$COMPLETE_KERNEL_NAME.tgz
	local EXE_DIR="$TOPDIR/src/colinux/os/winnt/build"
	
	echo "Create ZIP packages into $COLINUX_INSTALL_DIR"
	mkdir -p $COLINUX_INSTALL_DIR

	# remove old zip files
	rm -f $SYMBOLS_ZIP $DAEMONS_ZIP

	# Strip executables and put into ZIP file
	for i in $EXE_DIR/*.exe $EXE_DIR/*.sys
	do
		name=`basename $i`
		bname=`basename $i .exe`
		bname=`basename $bname .sys`
		oname=$COLINUX_INSTALL_DIR/$name
		
		# Create map file with symbols, add to zip
		map=$COLINUX_INSTALL_DIR/$bname.map
		$TARGET-nm $i | sort | uniq > $map
		zip -j $SYMBOLS_ZIP $map || exit $?
		rm $map

		# strip symbols and add exe file to zip
		$STRIP -o $oname $i || exit $?
		zip -j $DAEMONS_ZIP $oname || exit $?
		rm $oname
	done

	# Exist Kernel and is newer?
	if [ $COLINUX_TARGET_KERNEL_PATH/vmlinux -nt $VMLINUX_ZIP ]
	then
		echo "Installing Kernel $KERNEL_VERSION in $COLINUX_INSTALL_DIR"

		# remove old zip file
		rm -f $VMLINUX_ZIP

		# Add kernel to ZIP
		zip -j $VMLINUX_ZIP $COLINUX_TARGET_KERNEL_PATH/vmlinux || exit $?
	fi

	# Exist target modules.dep and is newer?
	if [ $COLINUX_TARGET_MODULE_PATH/lib/modules/$COMPLETE_KERNEL_NAME/modules.dep -nt $MODULES_TGZ ]
	then
		# Create compressed tar archive with path for extracting direct on the
		# root of fs, lib/modules with full version of kernel and colinux.
		echo "Installing Modules $KERNEL_VERSION in $COLINUX_INSTALL_DIR"
		cd "$COLINUX_TARGET_MODULE_PATH"
		tar cfz $MODULES_TGZ lib/modules/$COMPLETE_KERNEL_NAME || exit $?
	fi
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
    --package)
	build_package
	;;
    --help)
	echo "
	file: bin/build-common.sh

	Options: --build-all | --download-all | --rebuild-all | --help

	Whithout arguments is used for common set of variables and functions.
	For more information, read top of file."
	;;
esac
