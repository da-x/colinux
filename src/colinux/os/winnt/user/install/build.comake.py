def script_cmdline(scripter, tool_run_inf):
    nsis_install_path = r"C:\Program Files\NSIS\makensis"
    command_line = 'wine "%s" %s' % (
        nsis_install_path.replace("\\", "\\\\"),
        tool_run_inf.target.inputs[0].pathname)
    return command_line

targets['coLinux.exe'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.nsi'),
       Input('colinux_def.inc'),
    ],
)

def script_cmdline(scripter, tool_run_inf):
    command_line = 'python %s %s %s' % (
        tool_run_inf.target.inputs[0].pathname,
        tool_run_inf.target.inputs[1].pathname,
        tool_run_inf.target.pathname)
    return command_line

targets['colinux_def.inc'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux_def.py'),
       Input('src/colinux/VERSION', root_relative=True),
    ]
)

