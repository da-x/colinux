targets['daemon.o'] = Target(
    inputs=[
    Input('main.o'),
    Input('debug.o'),
    ],
)
