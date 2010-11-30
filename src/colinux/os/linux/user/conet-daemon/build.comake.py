# Strip out guest kernel include paths for tap.c builds
targets['tap.o'] = Target(
    inputs = [Input('tap.c')],
    tool = Compiler(),
    mono_options = Options(
        overriders = dict(
            compiler_includes = []
        )
    )
)

targets['build.o'] = Target(
    inputs=[
       Input('daemon.o'),
       Input('tap.o')
    ],
)
