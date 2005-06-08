targets['build.o'] = Target(
    inputs=[
       Input('daemon.o'),
       Input('tap-win32.o')
    ],
)
