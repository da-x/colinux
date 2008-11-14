targets['user.a'] = Target(
    inputs=[
    Input("user/user.a"),
    Input("common/common.a"),    
    ]
)

targets['user.o'] = Target(
    inputs=[
	Input("user/user.o"),
	Input("common/common.o"),
    ]
)
