
targets['core.c'] = Target(
    inputs = [Input("../core.c")],
    tool = Copy(),
)

# Create links to Makefile and all C-files in build directory
def script_cmdline(scripter, tool_run_inf):
    inputs = tool_run_inf.target.get_actual_inputs()
    command_line = (
     ('cp %s %s;'+
      'INFILE=%s;'+
      'INPATH=`dirname $INFILE`;'+
      'OUTPATH=%s_build;'+
      'mkdir -p $OUTPATH;'+
      'ln -sf `pwd`/$INFILE $OUTPATH/Makefile;'+
      'for name in $INPATH/*.c; do '+
        'OUTFILE=`basename $name`;'+
        'ln -sf `pwd`/$name $OUTPATH/$OUTFILE;'+
      'done') %
     (inputs[0].pathname, tool_run_inf.target.pathname,
      inputs[0].pathname, tool_run_inf.target.pathname))
    return command_line

targets['.module.arch'] = Target(
    inputs = [Input("../../../../arch/i386/Makefile.libm")],
    tool = Script(script_cmdline),
)

targets['.module.common'] = Target(
    inputs = [Input("../../../../common/Makefile.libm")],
    tool = Script(script_cmdline),
)

targets['.module.kernel'] = Target(
    inputs = [Input("../../../../kernel/Makefile.libm")],
    tool = Script(script_cmdline),
)

class ModuleBuilder(Executer):
    def prepare_command(self, tool_run_inf):
        from comake import build_root
        from os import putenv
        putenv('COLINUX_BASE', build_root)

    def get_command_line(self, tool_run_inf):
        return ('make V=0 -C $COLINUX_HOST_KERNEL_DIR '+
		'M=$COLINUX_BASE/src/colinux/os/linux/kernel/module ')

    def cleanup(self, tool_run_inf):
        import os
        os.unsetenv('COLINUX_BASE')

targets['colinux.ko'] = Target(
    inputs = [
        Input('core.c'),
        Input('.module.arch'),
        Input('.module.kernel'),
        Input('.module.common'),
    ],
    tool = ModuleBuilder(),
)
