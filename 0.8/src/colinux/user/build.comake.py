targets['user.a'] = Target(
    inputs=input_list(".c", ".o"),
)

targets['user.o'] = Target(
    inputs=input_list(".c", ".o"),
)
