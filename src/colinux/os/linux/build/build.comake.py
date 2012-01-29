targets['executables'] = Target(
    inputs = [
    Input('colinux.ko'),
    Input('colinux-daemon'),
    Input('colinux-net-daemon'),
    Input('colinux-slirp-net-daemon'),
    Input('colinux-debug-daemon'),
    Input('colinux-serial-daemon'),
    Input('colinux-console-fltk'),
    ],
    tool = Empty(),
)

def generate_options(compiler_def_type, libs=None, lib_paths=None):
    if not libs:
        libs = []
    if not lib_paths:
        lib_paths = []
    return Options(
        overriders = dict(
            compiler_def_type = compiler_def_type,
        ),
        appenders = dict(
            compiler_libs = libs,
            compiler_lib_paths = lib_paths,
        ),
    )

user_dep = [Input('../user/user-all.a')]

targets['colinux-daemon'] = Target(
    inputs = [
        Input('../user/daemon/daemon.o'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('gcc'),
)

targets['colinux-net-daemon'] = Target(
    inputs = [
       Input('../user/conet-daemon/build.o'),
       Input('../../../user/daemon-base/build.a'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('g++'),
)

targets['colinux-slirp-net-daemon'] = Target(
    inputs = [
       Input('../user/conet-slirp-daemon/build.o'),
       Input('../../../user/slirp/build.o'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('gcc'),
)

targets['colinux-console-fltk'] = Target(
    inputs = [
       Input('../user/console-fltk/build.o'),
       Input('../../../user/console-fltk/build.a'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('g++', libs=['X11', 'fltk'],
                                    lib_paths=['/usr/X11R6/lib']),
)

targets['colinux-debug-daemon'] = Target(
    inputs = [
       Input('../user/debug/build.o'),
       Input('../../../user/debug/build.o'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('gcc'),
)

targets['colinux-serial-daemon'] = Target(
    inputs = [
       Input('../user/coserial-daemon/build.o'),
    ] + user_dep,
    tool = Compiler(),
    mono_options = generate_options('gcc'),
)

targets['colinux.ko'] = Target(
    inputs = [Input('../kernel/module/colinux.ko')],
    tool = Copy(),
)
