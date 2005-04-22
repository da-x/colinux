#!/bin/sh

# (Contens are moved)
. ./build-common.sh --build-all $@

echo "
!!!	Script will be remove in future, please call in toplevel directory
!!!	'./configure && make && make install'
"
