# This is not a standalone Python script, but a build declaration file
# to be read by bin/make.py. Please run bin/make.py --help.

import os

from comake.settings import settings

settings.arch = os.getenv('COLINUX_ARCH')
settings.host_os = os.getenv('COLINUX_HOST_OS')

if not settings.arch:
    settings.arch = 'i386'
    print "Target architecture not specified, defaulting to %s" % (settings.arch, )    

current_arch_symlink = target_pathname(pathjoin('colinux', 'arch', 'current'))
if os.path.exists(current_arch_symlink):
    os.unlink(current_arch_symlink)
os.symlink(settings.arch, current_arch_symlink)

if not settings.host_os:
    settings.host_os = 'winnt'
    print "Target OS not specified, defaulting to %s" % (settings.host_os, )

current_os_symlink = target_pathname(pathjoin('colinux', 'os', 'current'))
if os.path.exists(current_os_symlink):
    os.unlink(current_os_symlink)
os.symlink(settings.host_os, current_os_symlink)

compiler_defines = dict(
    COLINUX_FILE_ID='0',
    COLINUX=None,
    CO_HOST_API=None,
    COLINUX_DEBUG=None,
    COLINUX_ARCH=settings.host_os,
)

if settings.host_os == 'winnt':
    cross_compilation_prefix = 'i686-pc-mingw32-'
    compiler_flags = ['-mpush-args', '-mno-accumulate-outgoing-args']
    compiler_defines['WINVER'] = '0x0500'
else:
    cross_compilation_prefix = ''
    compiler_flags = []

settings.target_kernel_path = getenv('COLINUX_TARGET_KERNEL_PATH')

if not settings.target_kernel_path:
    print 
    print "COLINUX_TARGET_KERNEL_PATH not set. Please set this environment variable to the"
    print "pathname of a coLinux-enabled kernel source tree, i.e, a Linux kenrel tree that"
    print "is patched with the patch file which is under the patch/ directory."
    raise BuildCancelError()

if not hasattr(settings, 'final_build_target'):
    settings.final_build_target = 'executables'
  
targets['build'] = Target(
    inputs=[Input('colinux/os/%s/build/%s' % (settings.host_os,
                                              settings.final_build_target))],
    options=Options(
        overriders=dict(
            cross_compilation_prefix=cross_compilation_prefix,
        ),
        appenders=dict(
            compiler_flags=[
                '-Wno-trigraphs', '-fno-strict-aliasing', '-Wall', '-O2',
            ] + compiler_flags,
            compiler_includes=[
                'src',
                pathjoin(settings.target_kernel_path, 'include'),
            ],
            compiler_defines=compiler_defines,
        )
    ),
    tool = Empty(),
)
