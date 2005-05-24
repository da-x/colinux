
# Remember: This file will be sourced from src/ as current directory.

# Grep kernel version
KERNEL_VERSION=`grep "^KERNEL_VERSION=" ../bin/build-common.sh | sed -e 's/[^0-9]*\([0-9\.]\+\).*/\1/'`

# coLinux Full version with "-preXX", if exist
PRE_VERSION=`cat $1`

# coLinux standard version
VERSION=`echo $PRE_VERSION | sed -e 's/\([^-]\+\).*/\1/'`

echo "!define VERSION \"$VERSION\""
echo "!define LONGVERSION \"$VERSION.1\""
echo "!define PRE_VERSION \"$PRE_VERSION\""
echo "!define KERNEL_VERSION \"$KERNEL_VERSION\""

# dist is s link to $COLINUX_INSTALL_DIR
echo "!define DISTDIR \"distdir\""
