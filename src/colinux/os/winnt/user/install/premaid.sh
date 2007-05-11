#!/bin/sh

# copy executable and text files into premaid for win installer

TOPDIR=../../../../../..
EXEDIR=../../build
THISDIR=`pwd`

# get configure
cd ${TOPDIR}/bin; . ./build-common.sh; cd $THISDIR

PATH="$PATH:$PREFIX/bin"
STRIP="$TARGET-strip --strip-all --preserve-dates"

# Convert files into win-lite CR+LF and store in distdir
# Set current CoLinux-Version number
unix_dos()
{
	sed "s/\$CO_VERSION/$CO_VERSION/" < $1 > $2
	unix2dos -q $2 || exit $?
}

mkdir -p premaid

# link kernel and modules
echo "Links to kernel and modules"
ln -f $COLINUX_TARGET_KERNEL_BUILD/vmlinux premaid/vmlinux || exit $?
ln -f $COLINUX_TARGET_KERNEL_BUILD/vmlinux-modules.tar.gz premaid/vmlinux-modules.tar.gz || exit $?

echo "Copy and strip executable"
for name in $EXEDIR/*.exe $EXEDIR/*.sys
do
	$STRIP -o premaid/`basename $name` $name
done

echo "Copy and convert text files"
unix_dos ${TOPDIR}/NEWS 		premaid/NEWS.txt
unix_dos ${TOPDIR}/RUNNING 		premaid/README.txt
unix_dos ${TOPDIR}/doc/cofs		premaid/cofs.txt
unix_dos ${TOPDIR}/doc/colinux-daemon	premaid/colinux-daemon.txt
unix_dos ${TOPDIR}/doc/debugging	premaid/debugging.txt
unix_dos ${TOPDIR}/conf/example.conf	premaid/example.conf
