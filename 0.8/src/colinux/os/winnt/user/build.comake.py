targets['user-all.a'] = Target(
    inputs=[
    Input("user-osdep.a"),
    Input("../../../user.a"),
    ]
)

targets['user-all.o'] = Target(
    inputs=[
    Input("user-osdep.o"),
    Input("../../../user.o"),
    ]
)

targets['user-osdep.a'] = Target(
    inputs=input_list(".c", ".o"),
)

targets['user-osdep.o'] = Target(
    inputs=input_list(".c", ".o"),
)
