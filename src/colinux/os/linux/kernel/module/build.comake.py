targets['prelinked_driver.o'] = Target(
    inputs = [
        Input('../core.o'),
        Input('../../../../arch/current/build.o'),
        Input('../../../../kernel/build.o'),
        Input('../../../../common/common.a'),
    ],
    tool = Linker(),
    options = Options(
        appenders = dict(
            compiler_defines = dict(
                __KERNEL__=None,
                CO_KERNEL=None,
                CO_HOST_KERNEL=None,
            ),
        )
    )
)

class ModuleBuilder(Executer):
    def prepare_command(self, tool_run_inf):
        from comake import build_root
        from os import putenv, getcwd, chdir
        putenv('COLINUX_PWD', pathjoin(build_root, 'src'))
        self.__prev_dir = getcwd()
        chdir(current_dirname)

    def get_command_line(self, tool_run_inf):
        return 'make V=1'

    def cleanup(self, tool_run_inf):
        import os
        os.chdir(self.__prev_dir)
        os.unsetenv('COLINUX_PWD')

targets['colinux.ko'] = Target(
    inputs = [
        Input('prelinked_driver.o', only_build_dep=True),
    ],
    tool = ModuleBuilder(),
)
