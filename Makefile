# Makefile for coLinux

# User config, if exist. Empty, if not.
USER_CFG := $(shell test -f bin/user-build.cfg \
    && echo "bin/user-build.cfg" )

ifneq ($(USER_CFG),)

# Detect target host OS (only if not as argument on make)
HOSTOS := $(shell . $(USER_CFG); echo $$COLINUX_HOST_OS)

# Target directories
export COLINUX_TARGET_KERNEL_SOURCE := \
    $(shell . bin/build-common.sh --get-vars; \
    echo $$COLINUX_TARGET_KERNEL_SOURCE)
export COLINUX_TARGET_KERNEL_BUILD := \
    $(shell . bin/build-common.sh --get-vars; \
    echo $$COLINUX_TARGET_KERNEL_BUILD)
export COLINUX_TARGET_KERNEL_PATH := \
    $(shell . bin/build-common.sh --get-vars; \
    echo $$COLINUX_TARGET_KERNEL_PATH)

else

X := $(shell echo "bin/user-build.cfg: Missing configured file." 1>&2 )

conf_missing:
	@echo "Build is not configured"
	@exit 1

endif

.PHONY: colinux clean distclean help

# Include host OS specific makefile
ifneq ($(HOSTOS),)
include Makefile.$(HOSTOS)

# Compile daemons
colinux:
	@cd src && make colinux

# Dump python build tree
dump:
	@cd src && python ../bin/make.py colinux --dump
endif

clean:
	make -C src clean
	find src \( -name '.tmp_versions' \
		-o -name '.module.*_build' \
		\) -type d -print | xargs rm -rf
	find src bin \( -name '*.o' -o -name '*.pyc' \
		-o -name '.*.cmd' -o -name 'colinux.mod.c' \
		-o -name 'Module.symvers' \
		\) -type f -print | xargs rm -f

distclean: clean
ifneq ($(USER_CFG),)
	rm -f bin/user-build.cfg bin/user-build.cfg.old \
	    $(shell . $(USER_CFG); echo $$BUILD_DIR)/.build-*.md5
endif
	find src \( -name 'premaid' \
		\) -type d -print | xargs rm -rf
	find src \( -name '*.orig' -o -name '*.rej' \
		-o -name '.*.orig' -o -name '.*.rej' \
		-o -name '*%' \
		\) -type f -print | xargs rm -f

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
	@echo  '  dump		- Dump build tree and dependencies from python scripts'
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
