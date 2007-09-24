
targets['core.c'] = Target(
    inputs = [Input("../core.c")],
    tool = Copy(),
)

targets['Makefile.colinux-objs'] = Target(
    inputs = input_list(".c", ".c") +
        [Input("core.c")],   # 'core.o' can be double, kbuild sorted it out
    tool = MakefileKbuild(),
)

# Create links to Makefile and all C-files in build directory
def script_cmdline(scripter, tool_run_inf):
    inputs = tool_run_inf.target.get_actual_inputs()
    from comake import build_root
    command_line = (
     ('INFILE=%s; '+
      'OUTFILE=%s; '+
      'BASE=%s; '+
      'INPATH=`dirname $INFILE`; '+
      'OUTPATH=${OUTFILE}_build; '+
      'mkdir -p $OUTPATH; '+
      'ln -sf $BASE/$INFILE $OUTPATH/Makefile; '+
      'for name in $INPATH/*.c; do '+
        'ln -sf $BASE/$name $OUTPATH/`basename $name`; '+
      'done; '+
      'cp $INFILE $OUTFILE') %
     (inputs[0].pathname, tool_run_inf.target.pathname,
      build_root))
    return command_line

targets['.module.arch'] = Target(
    inputs = [Input("../../../../arch/i386/Makefile.lib-m")],
    tool = Script(script_cmdline),
)

targets['.module.common'] = Target(
    inputs = [Input("../../../../common/Makefile.lib-m")],
    tool = Script(script_cmdline),
)

targets['.module.kernel'] = Target(
    inputs = [Input("../../../../kernel/Makefile.lib-m")],
    tool = Script(script_cmdline),
)

class ModuleBuilder(Executer):
    def prepare_command(self, tool_run_inf):
        from comake import build_root
        from os import putenv
        putenv('COLINUX_BASE', build_root)

    def get_command_line(self, tool_run_inf):
        return ('make V=0 -C $COLINUX_HOST_KERNEL_DIR '+
                'M=$COLINUX_BASE/%s' % current_dirname)

    def cleanup(self, tool_run_inf):
        import os
        os.unsetenv('COLINUX_BASE')

targets['colinux.ko'] = Target(
    inputs = [
        Input('Makefile.colinux-objs'),
        Input('.module.arch'),
        Input('.module.kernel'),
        Input('.module.common'),
        Input('../../../../common/version.h', only_build_dep=True),
    ],
    tool = ModuleBuilder(),
)
