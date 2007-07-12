#!/bin/sh

#
# This script builds the mingw32 cross compiler on Linux,
# and patches the w32api package.
#
# It also downloads, compiles, and installs FLTK.
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

# We need to find our self here, or in subdir bin
# BINDIR is bin directory with scripts
if [ -f bin/build-common.sh ]; then
	BINDIR=`pwd`/bin
elif [ -f build-common.sh ]; then
	BINDIR=`pwd`
else
	echo "build-common.sh: Can't detect bin directory!" >&2
	exit 1
fi

# TOPDIR is the directory with ./configure
TOPDIR=`dirname $BINDIR`

# Default Kernel version we are targeting, can overwrite in CFG file.
# Remember: Please update also conf/kernel-*-config
#
# Read version from filename patch/series-*, using the newest we found.
KERNEL_VERSION=`ls $TOPDIR/patch/series-* | sed -n -r -e 's/^.+-([0-9\.]+)$/\1/' -e '$p'`

# Use User config, if exist
# you probably don't need to change anything from here down
if [ -f $BINDIR/user-build.cfg ] ; then
	# Users directories
	. $BINDIR/user-build.cfg
else
	# fall back to default config
	. $BINDIR/sample.user-build.cfg
fi

# what flavor are we building?
TARGET=i686-pc-mingw32

# Current developing build system
BUILD=i686-pc-linux

# Updated by Sam Lantinga <slouken@libsdl.org>
# These are the files from the current MingW release

MINGW_VERSION="3.9"
MINGW_URL=http://heanet.dl.sourceforge.net/sourceforge/mingw
MINGW=mingw-runtime-$MINGW_VERSION
MINGW_ARCHIVE=$MINGW.tar.gz

BINUTILS_VERSION="2.16.91"
BINUTILS_RELEASE="$BINUTILS_VERSION-20060119-1"
BINUTILS=binutils-$BINUTILS_RELEASE
BINUTILS_ARCHIVE=$BINUTILS-src.tar.gz
#BINUTILS_PATCH="patch/$BINUTILS.diff"

GCC_VERSION="3.4.5"
GCC_RELEASE="$GCC_VERSION-20060117-1"
GCC=gcc-$GCC_RELEASE
GCC_ARCHIVE1=gcc-core-$GCC_RELEASE-src.tar.gz
GCC_ARCHIVE2=gcc-g++-$GCC_RELEASE-src.tar.gz
#GCC_PATCH="patch/$GCC.diff"

W32API_VERSION=3.5
W32API=w32api-$W32API_VERSION
W32API_SRC=$W32API
W32API_SRC_ARCHIVE=$W32API-src.tar.gz
W32API_ARCHIVE=$W32API.tar.gz
#W32API_PATCH="patch/$W32API_SRC.diff"


FLTK_VERSION="1.1.6"
FLTK_URL=http://heanet.dl.sourceforge.net/sourceforge/fltk 
FLTK=fltk-$FLTK_VERSION
FLTK_ARCHIVE=$FLTK-source.tar.bz2

WINPCAP_VERSION="3.1"
WINPCAP_URL=http://www.winpcap.org/archive
WINPCAP_SRC=WpdPack
WINPCAP_SRC_ARCHIVE=${WINPCAP_VERSION}-${WINPCAP_SRC}.zip


# KERNEL_VERSION: full kernel version (e.g. 2.6.11)
# KERNEL_DIR: sub-dir in www.kernel.org for the download (e.g. v2.6)
KERNEL_DIR=`echo $KERNEL_VERSION | sed -r -e 's/^([0-9]+)\.([0-9]+)\..+$/v\1.\2/'`

KERNEL=linux-$KERNEL_VERSION
KERNEL_URL=http://www.kernel.org/pub/linux/kernel/$KERNEL_DIR
KERNEL_ARCHIVE=$KERNEL.tar.bz2

CO_VERSION=`cat $TOPDIR/src/colinux/VERSION`
COMPLETE_KERNEL_NAME=$KERNEL_VERSION-co-$CO_VERSION

# Set defaults
if [ -z "$COLINUX_TARGET_KERNEL_SOURCE" -a -z "$COLINUX_TARGET_KERNEL_BUILD" ]
then
	if [ -z "$COLINUX_TARGET_KERNEL_PATH" ]
	then
		# Source and build in differ directories
		COLINUX_TARGET_KERNEL_SOURCE="$BUILD_DIR/$KERNEL-source"
		COLINUX_TARGET_KERNEL_BUILD="$BUILD_DIR/$KERNEL-build"
	else
		# fallback for old style
		COLINUX_TARGET_KERNEL_SOURCE="$COLINUX_TARGET_KERNEL_PATH"
		COLINUX_TARGET_KERNEL_BUILD="$COLINUX_TARGET_KERNEL_PATH"
	fi
fi

# MD5sum files stored here
MD5DIR="$BUILD_DIR"
W32LIBS_CHECKSUM="$MD5DIR/.build-colinux-libs.md5"
KERNEL_CHECKSUM="$MD5DIR/.build-kernel.md5"

# coLinux kernel we are targeting
if [ -z "$KERNEL_VERSION" -o -z "$KERNEL_DIR" ] ; then
    # What's wrong here?
    cat >&2 <<EOF
	Failed: \$KERNEL_VERSION or \$KERNEL_DIR
	Can't find the kernel patch, probably wrong script,
	or file patch/series-* don't exist?
EOF
    exit -1
fi

# Get variables only? Then end here.
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

# Default path to modules
if [ -z "$COLINUX_TARGET_MODULE_PATH" ] ; then
    COLINUX_TARGET_MODULE_PATH="$COLINUX_TARGET_KERNEL_BUILD/_install"
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
# Fairly for ccache: Add the cross path at end.

PATH="$PATH:$PREFIX/bin"

#
# download a file from a given url, only if it is not present
#

download_file()
{
	# Make sure wget is installed
	if [ "x`which wget`" = "x" ]
	then
		echo "You need to install wget." >&2
		exit 1
	fi

	if [ ! -f "$SOURCE_DIR/$1" ]
	then
		mkdir -p "$SOURCE_DIR"
		cd "$SOURCE_DIR"
		echo "Downloading $1"
		if ! wget "$2/$1"
		then
			echo "Could not download $1"
			# move broken download
			test -f $1 && mv $1 $1.incomplete
			exit 1
		fi
  		cd "$BINDIR"
	else
		echo "Found $1 in the srcdir $SOURCE_DIR"
	fi
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

#
# Strip kernel image file 'vmlinux'
# Arg1: input file
# Arg2: stripped output file
#
strip_kernel()
{
	local STRIP="strip --strip-all"
	local FROM_SOURCE="src/colinux/user/daemon.c"
	local KEEP

	# Build the list of needed symbols: Grep from loader function in daemon, this lines
	# --> rc = co_daemon_load_symbol(daemon, "init_thread_union", &import->kernel_init_task_union);
	# --> rc = co_daemon_load_symbol_and_data(daemon, "co_arch_info", &import->kernel_co_arch_info,
	#     _____^^^^^^^^^^^^^^^^^^^^^__________________^************^___^^^^^^______________________

	KEEP=`grep "co_daemon_load_symbol" $TOPDIR/$FROM_SOURCE | \
	  sed -n -r -e 's/^.+daemon.+\"(.+)\".+import.+$/ --keep-symbol=\1/p' | tr -d "\n"`
	if [ -n "$KEEP" ]
	then
		# Kernel strip
		$STRIP $KEEP -o $2 $1 || exit $?
	else
		# Function not found by grep
		echo -e "\nWARNING: $FROM_SOURCE" >&2
		echo -e "Can't get symbols for stripping! Don't strip vmlinux\n" >&2
			
		# Fallback into copy mode
		cp -a $1 $2
	fi
}

# Create ZIP packages (for "autobuild")
build_package()
{
	local name bname oname
	local STRIP="$TARGET-strip --strip-all"
	local DATE=`LANG=C TZ="UTC" date +%G%m%d`
	local SYMBOLS_ZIP=$COLINUX_INSTALL_DIR/daemons-$CO_VERSION-$DATE.dbg.zip
	local DAEMONS_ZIP=$COLINUX_INSTALL_DIR/daemons-$CO_VERSION-$DATE.zip
	local VMLINUX_ZIP=$COLINUX_INSTALL_DIR/vmlinux-$COMPLETE_KERNEL_NAME-$DATE.zip
	local MODULES_TGZ=$COLINUX_INSTALL_DIR/modules-$COMPLETE_KERNEL_NAME-$DATE.tgz
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

		# Add file with debugging symbols into zip
		zip -j "$SYMBOLS_ZIP" $i || exit $?
		
		# strip symbols and add file to zip
		$STRIP -o $oname $i || exit $?
		zip -j $DAEMONS_ZIP $oname || exit $?
		rm $oname
	done

	# Exist Kernel and is newer?
	if [ $COLINUX_TARGET_KERNEL_BUILD/vmlinux -nt $VMLINUX_ZIP ]
	then
		echo "Installing Kernel $KERNEL_VERSION in $COLINUX_INSTALL_DIR"

		# remove old zip file
		rm -f $VMLINUX_ZIP

		if [ "$COLINUX_KERNEL_STRIP" = "yes" ]
		then
			name=vmlinux
			oname=$COLINUX_INSTALL_DIR/$name

			# Create map file with symbols, add to zip
			map=$COLINUX_INSTALL_DIR/$name.map
			nm $COLINUX_TARGET_KERNEL_BUILD/$name | sort | uniq > $map
			zip -j $VMLINUX_ZIP $map || exit $?
			rm $map

			# Strip kernel and add to ZIP
			strip_kernel $COLINUX_TARGET_KERNEL_BUILD/$name $oname
			zip -j $VMLINUX_ZIP $oname || exit $?
			rm $oname
		else
			# Add kernel to ZIP (not stripped)
			zip -j $VMLINUX_ZIP $COLINUX_TARGET_KERNEL_BUILD/vmlinux || exit $?
		fi
	fi

	# Exist target modules.dep and is newer?
	if [ $COLINUX_TARGET_MODULE_PATH/lib/modules/$COMPLETE_KERNEL_NAME/modules.dep -nt $MODULES_TGZ ]
	then
		# Create compressed tar archive with path for extracting direct on the
		# root of fs, lib/modules with full version of kernel and colinux.
		echo "Installing Modules $KERNEL_VERSION in $COLINUX_INSTALL_DIR"
		cd "$COLINUX_TARGET_MODULE_PATH"
		tar czf $MODULES_TGZ lib/modules/$COMPLETE_KERNEL_NAME || exit $?
		cd -
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
