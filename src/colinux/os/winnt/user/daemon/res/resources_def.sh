#!/bin/sh

# Remember: This file runs from / as current directory.
# Standard arguments for this script:
# $1: src/colinux/VERSION
# $2: src/colinux/os/winnt/user/daemon/res/recources_def.inc

VERSION_FILE=$1
TARGET_FILE=$2

# Get kernel and gcc version
. bin/build-common.sh --get-vars

# coLinux full version with "-preXX"
CO_VERSION_STRING=`cat $VERSION_FILE`

# Get 4 digit words from full version
VERSION=`echo $CO_VERSION_STRING | sed -n -e 's/\([0-9\.]\+\)-[^0-9]*\([0-9]\+\).*/\1,\2/p'`

# Fallback without -preXX
test -z "$VERSION" && \
VERSION=`echo $CO_VERSION_STRING | sed -e 's/\([0-9\.]\+\).*/\1/'`.0

# Transform into comma format
CO_VERSION_DWORD=`echo $VERSION | sed -e 's/\./,/g'`

cat > $TARGET_FILE << EOF
#define CO_VERSION_STRING "$CO_VERSION_STRING"
#define CO_VERSION_DWORD $CO_VERSION_DWORD
#define CO_KERNEL_STRING "$KERNEL_VERSION"
#define CO_GCC_STRING "$GCC_VERSION"
EOF

