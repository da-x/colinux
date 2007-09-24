targets['common.a'] = Target(
    inputs=[
    Input('queue.o'),
    Input('console.o'),
    Input('debug.o'),
    Input('errors.o'),
    Input('messages.o'),
    Input('libc.o'),
    Input('libc_strtol.o'),
    Input('snprintf.o'),
    Input('file_ids.o'),
    Input('unicode.o'),
    ],
)

def file_id_inputs(scripter, tool_run_inf):
    command_line = 'echo "a.c" | python %s > %s' % (tool_run_inf.target.inputs[0].pathname,
                                tool_run_inf.target.pathname)
    return command_line

targets['file_ids.o'] = Target(
    tool = Compiler(),
    inputs = [Input('file_ids.c')],
)

class FileIdScript(Tool):
    def cache(self, target):
        return ''

    def _content(self):
        from StringIO import StringIO
        from comake.defaults import file_id_allocator
        output_file = StringIO()
        print >>output_file, "#include <stdlib.h>"
        print >>output_file, "const char *colinux_obj_filenames[] = {"
        for filename in file_id_allocator.path_list:
            print >>output_file, '     "%s", ' % (filename, )
        print >>output_file, ' NULL,'
        print >>output_file, "};"
        return output_file.getvalue()

    def make(self, target, reporter):
        output_file = open(target.pathname, 'wb')
        output_file.write(self._content())        

    def rebuild_needed(self, target):
        return open(target.pathname, 'rb').read() != self._content()

targets['file_ids.c'] = Target(
    tool = FileIdScript(),
)

def version_inputs(scripter, tool_run_inf):
    command_line = 'python %s %s %s' % (
        tool_run_inf.target.inputs[0].pathname,
        tool_run_inf.target.inputs[1].pathname,
        tool_run_inf.target.pathname)
    return command_line

targets['version.h'] = Target(
    tool = Script(version_inputs),
    inputs = [
       Input('version.py'),
       Input('../VERSION'),
    ]
)

target = 'libc.o'
targets[target] = target = deftarget(target)
target.mono_options = Options(
    appenders = dict(
    compiler_defines = dict(CO_LIBC__MISC=None),
    )
)

targets['libc_strtol.o'] = target = deftarget('libc.o')
target.mono_options = Options(
    appenders = dict(
    compiler_defines = dict(CO_LIBC__STRTOL=None),
    )
)

targets['Makefile.libm'] = Target(
    inputs = input_list(".c", ".c"),
    tool = MakefileKbuild(),
    mono_options = Options(
	appenders = dict(compiler_defines = dict(CO_LIBC__MISC=None),)
    )
)
