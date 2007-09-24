targets['build.o'] = Target(
    inputs = input_list(".c", ".o"),
)

targets['Makefile.libm'] = Target(
    inputs = input_list(".c", ".c"),
    tool = MakefileKbuild(),
)
