#!/bin/sh

# This File exports the current version informations and
# creates an NIS installer include file.

# Remember: This file will be sourced from / as current directory.
# Standard arguments for this script:
# $1: src/colinux/VERSION
# $2: src/colinux/os/winnt/user/install/colinux_def.inc

VERSION_FILE=$1
TARGET_FILE=$2

# Fallback for dash with defaults
test -z "$VERSION_FILE" && VERSION_FILE="src/colinux/VERSION"
test -z "$TARGET_FILE" && TARGET_FILE="src/colinux/os/winnt/user/install/colinux_def.inc"

# Get kernel version
. bin/build-common.sh --get-vars

# coLinux full version with "-preXX", if exist
PRE_VERSION=`cat $VERSION_FILE`

# coLinux standard version
VERSION=`echo $PRE_VERSION | sed -e 's/\([^-]*\).*/\1/'`

cat > $TARGET_FILE << EOF
!define VERSION "$VERSION"
!define LONGVERSION "$VERSION.1"
!define PRE_VERSION "$PRE_VERSION"
!define KERNEL_VERSION "$KERNEL_VERSION"
EOF
