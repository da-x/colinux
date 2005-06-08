#!/bin/sh

# (Contens are moved)
source build-common.sh --build-all $@

echo "
!!!	Script build-all.sh will be remove in future, please run
!!!	'./configure && make && make installer' in toplevel directory
"
