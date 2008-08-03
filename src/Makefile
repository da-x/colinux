#
# This source code is a part of coLinux source package.
# 
# Dan Aloni <da-x@gmx.net>, 2003 (c)
#
# gmake-compatible old build interface.
#

.phony: FORCE 

FORCE:

colinux: FORCE 
	python ../bin/make.py colinux

installer:
	python ../bin/make.py installer

clean:
	python ../bin/make.py clean
	rm -f colinux/arch/current colinux/os/current
	rm -f src/colinux/os/darwin/kernel/build
	rm -f src/colinux/os/darwin/linuxheaders
