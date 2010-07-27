def script_cmdline(scripter, tool_run_inf):
    inputs = tool_run_inf.target.get_actual_inputs()
    exe_name_option = tool_run_inf.options.get('exe_name_option', "")
    text_name_option = tool_run_inf.options.get('text_name_option', "")
    command_line = ((
        "%s -O coff -I %s %s -o %s '-DCO_FILENAME_STRING=\\\"colinux%s.exe\\\"' '-DCO_TEXTNAME_STRING=\\\"coLinux%s program\\\"'"
        ) %
    (scripter.get_cross_build_tool('windres', tool_run_inf),
     current_dirname,
     inputs[0].pathname,
     tool_run_inf.target.pathname,
     exe_name_option,
     text_name_option,))
    return command_line

targets['daemon.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('daemon.rc'),
       Input('resources_def.inc'),
    ],
)

targets['colinux-debug.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-debug-daemon",
            text_name_option = " Debug daemon",
        )
    )
)

targets['colinux-net.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-net-daemon",
            text_name_option = " TAP Network daemon",
        )
    )
)

targets['colinux-bridged-net.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-bridged-net-daemon",
            text_name_option = " PCAP Bridged Network daemon",
        )
    )
)

targets['colinux-ndis-net.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-ndis-net-daemon",
            text_name_option = " Kernel Bridged Network daemon",
        )
    )
)

targets['colinux-slirp-net.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-slirp-net-daemon",
            text_name_option = " SLiRP Network daemon",
        )
    )
)

targets['colinux-serial.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-serial-daemon",
            text_name_option = " Serial daemon",
        )
    )
)

targets['colinux-fltk.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-console-fltk",
            text_name_option = " FLTK console",
        )
    )
)

targets['colinux-nt.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-console-nt",
            text_name_option = " NT console",
        )
    )
)

targets['colinux-wx.res'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('colinux.rc'),
       Input('resources_def.inc'),
    ],
    options = Options(
        appenders = dict(
            exe_name_option = "-console-wx",
            text_name_option = " WX console",
        )
    )
)

def script_cmdline(scripter, tool_run_inf):
    command_line = '%s %s %s' % (
        tool_run_inf.target.inputs[0].pathname,
        tool_run_inf.target.inputs[1].pathname,
        tool_run_inf.target.pathname)
    return command_line

targets['resources_def.inc'] = Target(
    tool = Script(script_cmdline),
    inputs = [
       Input('resources_def.sh'),
       Input('src/colinux/VERSION', root_relative=True),
       Input('bin/build-common.sh', root_relative=True),
    ]
)
