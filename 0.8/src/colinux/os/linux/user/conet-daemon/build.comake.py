targets['build.o'] = Target(
    inputs=[
       Input('daemon.o'),
       Input('tap.o')
    ],
)
