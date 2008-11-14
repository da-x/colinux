targets['build.a'] = Target(
    inputs=input_list(".cpp", ".o"),
)

targets['build.o'] = Target(
    inputs=input_list(".cpp", ".o"),
)
