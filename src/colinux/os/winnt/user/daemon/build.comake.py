targets['daemon.o'] = Target(
    inputs=[
    Input('main.o'),
    Input('debug.o'),
    Input('service.o'),
    Input('driver.o'),
    Input('cmdline.o'),
    Input('misc.o'),
    ],
)
