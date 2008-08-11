targets['build.a'] = Target(
    inputs=[
       Input('daemon.o'),
       Input('tap-win32.o')
    ],
)
