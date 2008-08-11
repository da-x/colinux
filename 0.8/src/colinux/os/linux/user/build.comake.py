targets['user-all.a'] = Target(
    inputs=[
    Input("user-osdep.a"),
    Input("../../../user.a"),
    ]
)

targets['user-osdep.a'] = Target(
    inputs=input_list(".c", ".o"),
)
