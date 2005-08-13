# Makefile for coLinux

# User config, if exist. Fallback to sample, if not.
USER_CFG := $(shell test -f bin/user-build.cfg \
    && echo "bin/user-build.cfg" )

ifneq ($(USER_CFG),)

# Detect target host OS (only if not as argument on make)
HOSTOS := $(shell . $(USER_CFG); echo $$COLINUX_HOST_OS)

# Target directories
export COLINUX_TARGET_KERNEL_PATH := $(shell . $(USER_CFG); \
    echo $$COLINUX_TARGET_KERNEL_PATH)

else

X := $(shell echo "bin/user-build.cfg: Missing configured file." 1>&2 )

conf_missing:
	@echo "Build is not configured"
	@exit 1

endif

# Include host OS specific makefile
ifneq ($(HOSTOS),)
include Makefile.$(HOSTOS)
endif

.PHONY: colinux clean distclean help
# Compile daemons
colinux:
	@cd src && make colinux

clean:
	@cd src && make clean

distclean: clean
ifneq ($(USER_CFG),)
	rm -f bin/user-build.cfg bin/user-build.cfg.old \
	 $(shell . $(USER_CFG); echo $$BUILD_DIR)/.build-*.md5
endif

help:
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		- Build crosstools, all targets marked with [*]'
	@echo  '* download	- Download all sources (for cross tools and libs)'
	@echo  '* cross		- Build and install cross compiler and binutils'
	@echo  '* libs		- Build and install libs: fltk,win32api'
	@echo  '* kernel	- Build colinux kernel vmlinux and modules'
	@echo  '* colinux	- Build colinux daemons'
	@echo  '  package	- Create ZIP file packages for pre releases'
	@echo  '  installer	- Create Installer (need wine and running x11)'
	@echo  ''
	@echo  'Cleaning colinux build (daemons only, no cross compiler, no libs):'
	@echo  '  clean		- remove most generated files but keep distry files'
	@echo  '  distclean	- remove all generated files + config + md5sum'
	@echo  ''
	@echo  'Options'
	@echo  '  HOSTOS=winnt  - Build targets for Winnt'
	@echo  '  HOSTOS=linux  - Build targets for Linux as host'
ifeq ($(USER_CFG),)
	@echo  ''
	@echo  'Please configure directories in file bin/user-build.cfg and use'
	@echo  'bin/sample.user-build.cfg as sample!  Or use "./configure --help"'
endif
# Makefile for coLinux

# User config, if exist. Fallback to sample, if not.
USER_CFG := $(shell test -f bin/user-build.cfg \
    && echo "bin/user-build.cfg" )

ifneq ($(USER_CFG),)

# Detect target host OS (only if not as argument on make)
HOSTOS := $(shell . $(USER_CFG); echo $$COLINUX_HOST_OS)

# Target directories
export COLINUX_TARGET_KERNEL_PATH := $(shell . $(USER_CFG); \
    echo $$COLINUX_TARGET_KERNEL_PATH)

else

X := $(shell echo "bin/user-build.cfg: Missing configured file." 1>&2 )

conf_missing:
	@echo "Build is not configured"
	@exit 1

endif

# Include host OS specific makefile
ifneq ($(HOSTOS),)
include Makefile.$(HOSTOS)
endif

.PHONY: colinux clean distclean help
# Compile daemons
colinux:
	@cd src && make colinux

clean:
	@cd src && make clean

distclean: clean
ifneq ($(USER_CFG),)
	rm -f bin/user-build.cfg bin/user-build.cfg.old \
	 $(shell . $(USER_CFG); echo $$BUILD_DIR)/.build-*.md5
endif

help:
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		- Build crosstools, all targets marked with [*]'
	@echo  '* download	- Download all sources (for cross tools and libs)'
	@echo  '* cross		- Build and install cross compiler and binutils'
	@echo  '* libs		- Build and install libs: fltk,win32api'
	@echo  '* kernel	- Build colinux kernel vmlinux and modules'
	@echo  '* colinux	- Build colinux daemons'
	@echo  '  package	- Create ZIP file packages for pre releases'
	@echo  '  installer	- Create Installer (need wine and running x11)'
	@echo  ''
	@echo  'Cleaning colinux build (daemons only, no cross compiler, no libs):'
	@echo  '  clean		- remove most generated files but keep distry files'
	@echo  '  distclean	- remove all generated files + config + md5sum'
	@echo  ''
	@echo  'Options'
	@echo  '  HOSTOS=winnt  - Build targets for Winnt'
	@echo  '  HOSTOS=linux  - Build targets for Linux as host'
ifeq ($(USER_CFG),)
	@echo  ''
	@echo  'Please configure directories in file bin/user-build.cfg and use'
	@echo  'bin/sample.user-build.cfg as sample!  Or use "./configure --help"'
endif
# Makefile for coLinux 0.7.1 (mingw32)
# created by configure at Mon May 30 16:44:41 CDT 2005

TARGET=i686-pc-mingw32
USER_TOPDIR=/home/george/src
SRCDIR=/home/george/src/co-devel
DOWNLOAD=/home/george/dls
BUILDDIR=/home/george/dls
PREFIX=/home/george/i686-pc-mingw32
DISTDIR=/home/george/build
MD5CACHE=/home/george/tmp

# Target directories
export COLINUX_TARGET_KERNEL_PATH=/home/george/dls/linux-2.6.11.11
export COLINUX_INSTALL_DIR=$(DISTDIR)

# Need some variables and PATH of cross compiler in make. Export this all
export PATH := $(PREFIX)/bin:${PATH}

.PHONY: all cross cross libs kernel download package installer
all: cross libs kernel colinux

#
# Check tools and targets via md5sum
#

cross:
	@cd bin && ./build-cross.sh

libs:
	@cd bin && ./build-colinux-libs.sh

kernel:
	@cd bin && ./build-kernel.sh

# Download only all missing sources (for cross compile)
download:
	@cd bin && source build-common.sh --download-all

# Create a pre-distributabel package as ZIP
package:
	@cd bin && source build-common.sh --package-all

# Create installer (need wine)
installer:
	@cd src && make installer

.PHONY: colinux clean help distclean
# Compile daemons
colinux:
	@cd src && make colinux

clean:
	@cd src && make clean

help:
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets'
	@echo  'Execute "make package" to ZIP targets into $(DISTDIR)'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		- Build crosstools, all targets marked with [*]'
	@echo  '* download	- Download all sources (for cross tools and libs)'
	@echo  '* cross		- Build and install cross compiler and binutils'
	@echo  '* libs		- Build and install libs: fltk,win32api'
	@echo  '* kernel	- Build colinux kernel vmlinux and modules'
	@echo  '* colinux	- Build colinux daemons'
	@echo  '  package	- Create ZIP file packages for pre releases'
	@echo  '  installer	- Create Installer (need wine)'
	@echo  ''
	@echo  'Cleaning colinux build (daemons only, no cross compiler, no libs):'
	@echo  '  clean		- remove most generated files but keep the config and distry files'
	@echo  '  distclean	- remove all generated files + config + md5sum'
	@echo  ''

distclean: clean
	rm -f Makefile Makefile.old \
	 bin/user-build.cfg bin/user-build.cfg.old \
	 $(MD5CACHE)/.build-*.md5

######################################
# CONFIGURE was call with follow args:
# --target=i686-pc-mingw32 --colinux-os=winnt --srcdir=/home/george/src/co-devel --topdir=/home/george/src --downloaddir=/home/george/dls --builddir=/home/george/tmp --prefix=/home/george/i686-pc-mingw32 --distdir=/home/george/build --targetkerneldir=/home/george/dls/linux-2.6.11.11 --targetmoduledir=/home/george/dls/linux-2.6.11.11/_install --configfile=bin/user-build.cfg --logfile=/home/george/log/co-devel-501.log
