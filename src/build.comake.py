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
os.unlink(current_arch_symlink)
os.symlink(settings.arch, current_arch_symlink)

if not settings.host_os:
    settings.host_os = 'winnt'
    print "Target OS not specified, defaulting to %s" % (settings.host_os, )

current_os_symlink = target_pathname(pathjoin('colinux', 'os', 'current'))
os.unlink(current_os_symlink)
os.symlink(settings.host_os, current_os_symlink)

if settings.host_os == 'winnt':
    cross_compilation_prefix = 'i686-pc-mingw32-'
    compiler_flags = ['-mpush-args', '-mno-accumulate-outgoing-args']
else:
    cross_compilation_prefix = ''
    compiler_flags = []

targets['build'] = Target(
    inputs=[Input('colinux/os/%s/build/executables' % (settings.host_os, ))],
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
                pathjoin(getenv('COLINUX_TARGET_KERNEL_PATH'), 'include'),
            ],
            compiler_defines=dict(
                COLINUX_FILE_ID='0',
                WINVER='0x0500',
                COLINUX=None,
                CO_HOST_API=None,
                COLINUX_DEBUG=None,
                COLINUX_ARCH=settings.host_os,
            )
        )
    ),
    tool = Empty(),
)
