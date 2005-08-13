#!/bin/sh

# copy executable and text files into premaid for win installer

TOPDIR=../../../../../..
EXEDIR=../../build
THISDIR=`pwd`

# get configur
cd ${TOPDIR}/bin; . build-common.sh; cd $THISDIR

PATH="$PREFIX/bin:$PATH"
STRIP="$TARGET-strip --strip-all"

# Convert files into win-lite CR+LF and store in distdir
unix_dos()
{
	cp $1 $2
	unix2dos -q $2
}

mkdir -p premaid

# link kernel
ln -f $COLINUX_TARGET_KERNEL_PATH/vmlinux premaid/vmlinux

# Create compressed tar archive with path for extracting direct on the
# root of fs, lib/modules with full version of kernel and colinux.
echo "Installing Modules $KERNEL_VERSION"
cd $COLINUX_TARGET_MODULE_PATH
tar cfz $THISDIR/premaid/vmlinux-modules.tar.gz lib/modules/$COMPLETE_KERNEL_NAME || exit $?
cd $THISDIR

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
unix_dos ${TOPDIR}/doc/example.conf	premaid/example.conf
#!/bin/sh

# copy executable and text files into premaid for win installer

TOPDIR=../../../../../..
EXEDIR=../../build
THISDIR=`pwd`

# get configur
cd ${TOPDIR}/bin; . build-common.sh; cd $THISDIR

PATH="$PREFIX/bin:$PATH"
STRIP="$TARGET-strip --strip-all"

# Convert files into win-lite CR+LF and store in distdir
unix_dos()
{
	cp $1 $2
	unix2dos -q $2
}

mkdir -p premaid

# link kernel
ln -f $COLINUX_TARGET_KERNEL_PATH/vmlinux premaid/vmlinux

# Create compressed tar archive with path for extracting direct on the
# root of fs, lib/modules with full version of kernel and colinux.
echo "Installing Modules $KERNEL_VERSION"
cd $COLINUX_TARGET_MODULE_PATH
tar cfz $THISDIR/premaid/vmlinux-modules.tar.gz lib/modules/$COMPLETE_KERNEL_NAME || exit $?
cd $THISDIR

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
unix_dos ${TOPDIR}/doc/example.conf	premaid/example.conf
