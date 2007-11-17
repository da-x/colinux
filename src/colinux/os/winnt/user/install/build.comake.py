def script_cmdline(scripter, tool_run_inf):
    nsis_install_path = r"C:\Program Files\NSIS\makensis"
    command_line = 'wine "%s" /V3 %s' % (
        nsis_install_path.replace("\\", "\\\\"),
        tool_run_inf.target.inputs[0].pathname)
    return command_line

targets['coLinux.exe'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.nsi'),
       Input('colinux_def.inc'),
       Input('WinpcapRedir.ini'),
       Input('iDl.ini'),
       Input('premaid/colinux-bridged-net-daemon.exe'),
       Input('premaid/colinux-console-fltk.exe'),
       Input('premaid/colinux-console-nt.exe'),
       Input('premaid/colinux-daemon.exe'),
       Input('premaid/colinux-net-daemon.exe'),
       Input('premaid/colinux-slirp-net-daemon.exe'),
       Input('premaid/linux.sys'),
       Input('premaid/README.txt'),
       Input('premaid/cofs.txt'),
       Input('premaid/colinux-daemon.txt'),
       Input('premaid/example.conf'),
       Input('premaid/vmlinux'),
       Input('premaid/vmlinux-modules.tar.gz'),
       Input('premaid/initrd.gz'),
    ],
)

def script_cmdline(scripter, tool_run_inf):
    command_line = 'sh -c ". %s %s %s"' % (
        tool_run_inf.target.inputs[0].pathname,
        tool_run_inf.target.inputs[1].pathname,
        tool_run_inf.target.pathname)
    return command_line

targets['colinux_def.inc'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux_def.sh'),
       Input('src/colinux/VERSION', root_relative=True),
    ]
)
