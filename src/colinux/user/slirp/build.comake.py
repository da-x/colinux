targets['build.o'] = Target(
    inputs=input_list(".c", ".o"),
    options=Options(
        overriders=dict(
            compiler_optimization="-O0",
        )
    )
)
