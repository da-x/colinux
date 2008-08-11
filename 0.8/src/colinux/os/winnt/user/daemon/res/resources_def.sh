#!/bin/sh

# Remember: This file runs from / as current directory.
# Standard arguments for this script:
# $1: src/colinux/VERSION
# $2: src/colinux/os/winnt/user/daemon/res/resources_def.inc

VERSION_FILE=$1
TARGET_FILE=$2

# Get kernel and gcc version
. bin/build-common.sh --get-vars

# coLinux full version with "-preXX" and build date
CO_VERSION_STRING=`cat $VERSION_FILE`-`date +%Y%m%d`

# Get 4 digit words from full version
VERSION=`sed -n -e 's/\([0-9\.]\+\)-[^0-9]*\([0-9]\+\).*/\1,\2/p' < $VERSION_FILE`

# Fallback 3 digit words without -preXX
test -z "$VERSION" && \
VERSION=`sed -e 's/\([0-9\.]\+\).*/\1/' < $VERSION_FILE`.0

# Transform into comma format
CO_VERSION_DWORD=`echo $VERSION | sed -e 's/\./,/g'`

# Get gcc version from current running (not from config file defined)
GCC_VERSION_STRING="`$TARGET-gcc -dumpversion`"

cat > $TARGET_FILE << EOF
#define CO_VERSION_STRING "$CO_VERSION_STRING"
#define CO_VERSION_DWORD $CO_VERSION_DWORD
#define CO_KERNEL_STRING "$KERNEL_VERSION"
#define CO_GCC_STRING "$GCC_VERSION_STRING"
EOF

