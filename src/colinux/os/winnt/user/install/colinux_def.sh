#!/bin/sh

# Remember: This file will be sourced from / as current directory.
# Standard arguments for this script:
# pwd: $HOME/CoLinux/work/co-devel
# $0: /bin/sh
# $1: src/colinux/VERSION
# $2: src/colinux/os/winnt/user/install/colinux_def.inc

VERSION_FILE=$1
TARGET_FILE=$2

# Get kernel version
KERNEL_VERSION=`cd bin; . build-common.sh --get-vars; echo \$KERNEL_VERSION`

# coLinux full version with "-preXX", if exist
PRE_VERSION=`cat $1`

# coLinux standard version
VERSION=`echo $PRE_VERSION | sed -e 's/\([^-]\+\).*/\1/'`

cat > $TARGET_FILE << EOF
!define VERSION "$VERSION"
!define LONGVERSION "$VERSION.1"
!define PRE_VERSION "$PRE_VERSION"
!define KERNEL_VERSION "$KERNEL_VERSION"
EOF
#!/bin/sh

# Remember: This file will be sourced from / as current directory.
# Standard arguments for this script:
# pwd: $HOME/CoLinux/work/co-devel
# $0: /bin/sh
# $1: src/colinux/VERSION
# $2: src/colinux/os/winnt/user/install/colinux_def.inc

VERSION_FILE=$1
TARGET_FILE=$2

# Get kernel version
KERNEL_VERSION=`cd bin; . build-common.sh --get-vars; echo \$KERNEL_VERSION`

# coLinux full version with "-preXX", if exist
PRE_VERSION=`cat $1`

# coLinux standard version
VERSION=`echo $PRE_VERSION | sed -e 's/\([^-]\+\).*/\1/'`

cat > $TARGET_FILE << EOF
!define VERSION "$VERSION"
!define LONGVERSION "$VERSION.1"
!define PRE_VERSION "$PRE_VERSION"
!define KERNEL_VERSION "$KERNEL_VERSION"
EOF
