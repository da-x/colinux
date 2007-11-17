targets['user-all.a'] = Target(
    inputs=[
    Input("user-osdep.a"),
    Input("../../../user.a"),
    ]
)

targets['user-osdep.a'] = Target(
    inputs=[
    Input("manager.o"),
    Input("file.o"),
    Input("file-write.o"),
    Input("file-unlink.o"),
    Input("alloc.o"),
    Input("misc.o"),
    Input("osdep.o"),
    Input("timer.o"),
    Input("exec.o"),
    Input("udp.o"),
    Input("cobdpath.o"),
    Input("reactor.o"),
    Input("process.o"),
    ]
)
