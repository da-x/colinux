#!/bin/sh

# This is my script for building a complete cross-compiler toolchain.
# It is based partly on Ray Kelm's script, which in turn was built on
# Mo Dejong's script for doing the same, but with some added fixes.
# The intent with this script is to build a cross-compiled version
# of the current MinGW environment.
#
# Updated by Sam Lantinga <slouken@libsdl.org>

# what flavor are we building?
TARGET=i686-pc-mingw32

# where does it go?
if [ "$PREFIX" = "" ] ; then
    echo "Please specify the $""PREFIX directory (e.g, /usr/local/mingw32)"
    exit -1
fi

# where does it go?
if [ "$SOURCE_DIR" = "" ] ; then
    echo "Please specify the $""SOURCE_DIR directory (e.g, /tmp/$USER/download)"
    exit -1
fi

# you probably don't need to change anything from here down
TOPDIR=`pwd`
SRCDIR="$SOURCE_DIR"

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

W32API=w32api-2.5
MINGW_URL=http://heanet.dl.sourceforge.net/sourceforge/mingw
