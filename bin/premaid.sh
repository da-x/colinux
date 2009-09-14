#!/bin/sh

# copy executable and text files into premaid for win installer

. ./build-common.sh

EXEDIR="$TOPDIR/src/colinux/os/winnt/build"
PREMAID="$TOPDIR/src/colinux/os/winnt/user/install/premaid"

# Update only on request
if [ "$1" = "--update" ]
then
	isok=true
	for name in $EXEDIR/*.exe $EXEDIR/*.sys \
		$COLINUX_TARGET_KERNEL_BUILD/vmlinux \
		$COLINUX_TARGET_KERNEL_BUILD/vmlinux-modules.tar.gz
	do
		if [ $PREMAID/`basename $name` -ot $name ]
		then
			isok=false
		fi
	done

	if $isok
	then
		echo "Premaid is up to date"
		exit 0
	fi
fi

PATH="$PATH:$PREFIX/bin"
STRIP="$TARGET-strip --strip-all --preserve-dates"

# Convert files into win-lite CR+LF and store in distdir
# Set current CoLinux-Version number
unix_dos()
{
	sed -e "s/\$CO_VERSION/$CO_VERSION/" \
	    -e 's/$/\r/' < $1 > $2 || exit $?
}

mkdir -p $PREMAID

# link kernel and modules
echo "Links to kernel and modules"
ln -f $COLINUX_TARGET_KERNEL_BUILD/vmlinux $PREMAID/vmlinux || exit $?
ln -f $COLINUX_TARGET_KERNEL_BUILD/vmlinux-modules.tar.gz $PREMAID/vmlinux-modules.tar.gz || exit $?

echo "Copy and strip executable"
for name in $EXEDIR/*.exe $EXEDIR/*.sys
do
	$STRIP -o $PREMAID/`basename $name` $name
done

echo "Copy and convert text files"
unix_dos ${TOPDIR}/NEWS 		$PREMAID/NEWS.txt
unix_dos ${TOPDIR}/RUNNING 		$PREMAID/README.txt
unix_dos ${TOPDIR}/doc/cofs		$PREMAID/cofs.txt
unix_dos ${TOPDIR}/doc/colinux-daemon	$PREMAID/colinux-daemon.txt
unix_dos ${TOPDIR}/doc/debugging	$PREMAID/debugging.txt
unix_dos ${TOPDIR}/conf/example.conf	$PREMAID/example.conf
