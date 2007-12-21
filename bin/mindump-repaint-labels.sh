#!/bin/sh

# Use this script to repaint BSOD files with coLinux label names. This Script
# reads text from "!analyze -v" and prints stack entries with label names.
#
# Usage:
#	mindump-repaint-labels.sh [linux.sys] < minidump.txt > backtrace.txt
#
# Needs the file linux.sys with debugging information. This file exist
# in zip archive daemons-*.dbg.zip, or in project build directory
# src/colinux/os/winnt/build. The file must give it as first parameter
# to the script, or should exist in currenty directory. Please use a file
# from exactly same build, that triggered the minidump.
#
# Script input and outputs are stdin and stdout, please use redirections.
#
# Needs installed tools i686-pc-mingw32-nm (from coLinux build system, native
# nm also usable), bc (command line calcualtor), sed, tr, cut, paste.
#
# Example from "!analyze -v" of minidump, before converted...
# STACK_TEXT:  
# b090bb38 ad06a8a2 89ff9278 00000000 000000a2 nt!KeReleaseMutex+0x14
# b090bb58 ad063e6b 89ff9278 00000000 b090bb98 linux+0xa8a2
# b090bb78 ad06451e 87d2cea0 87e98f90 000000a2 linux+0x3e6b
#
# Converted into...
# b090bb38 ad06a8a2 89ff9278 00000000 000000a2 nt!KeReleaseMutex+0x14
# b090bb58 ad063e6b 89ff9278 00000000 b090bb98 linux!co_os_mutex_release+0x12 (linux+0xa8a2)
# b090bb78 ad06451e 87d2cea0 87e98f90 000000a2 linux!put_section+0x2B (linux+0x3e6b)
# 
# Henry Nestler (c), 2007
# #######################

# Path to mingw32 binutils (i686-pc-mingw32-nm)
PATH="$HOME/colinux/mingw32/bin:$PATH"

# List symbols from object files
NM=i686-pc-mingw32-nm

# Fallback to localy NM, if cross nm is not usable
which $NM >/dev/null 2>&1 || NM=nm

# Base for tempfiles
NMFILE="/tmp/linux.sys.$$.nm"

if [ -n "$1" ]
then
    LINUXSYS="$1"
else
    LINUXSYS="linux.sys"
fi

if [ ! -f $LINUXSYS ]
then
    echo "$LINUXSYS: Missing file linux.sys, please give such as first parameter." >&2
    exit 1
fi

# Create list with all labels (reverse sorted)
$NM -r -n -C $LINUXSYS | uniq > $NMFILE
if [ ! -s $NMFILE ]
then
    rm $NMFILE
    exit 1
fi

# Convert address into decimal
( echo "ibase=16"; cut -d ' ' -f 1 < $NMFILE | tr [[:lower:]] [[:upper:]] ) | bc > $NMFILE.numbers
cut -d ' ' -f 3 < $NMFILE > $NMFILE.names
paste $NMFILE.numbers $NMFILE.names > $NMFILE.dec
rm $NMFILE $NMFILE.numbers $NMFILE.names

# Search start address and label name for given offset
search_label()
{
    while read address name
    do
	if [ $address -le $1 ]
	then
	    echo "$address $name"
	    return
	fi
    done < $NMFILE.dec
}

# Serach and replace all "linux+0x..." with label names and offsets
start=false
sed -e 's/\r//' |\
while read s1 s2 s3 s4 s5 name
do
    if [ "$s1" = "STACK_TEXT:" ]
    then
	start=true
    elif [ -z "$s1" ]
    then
	start=false
    elif $start
    then
	offset=`echo "$name" | sed -n -r -e 's/^linux\+0x(.+)$/\1/p' | tr [[:lower:]] [[:upper:]]`
	if [ -n "$offset" ]
	then
	    # Add offset 0x10000 and convert from hex into decimal
	    value=`echo -e "ibase=16\n10000+$offset" | bc`
	    set -- `search_label $value`
	    address=$1
	    label=$2
	    diff=`echo -e "obase=16\n$value-$address" | bc`
	    echo "$s1 $s2 $s3 $s4 $s5 linux!$label+0x$diff	($name)"
	else
	    echo "$s1 $s2 $s3 $s4 $s5 $name"
	fi
    fi
done

# Remove temp file
rm $NMFILE.dec
