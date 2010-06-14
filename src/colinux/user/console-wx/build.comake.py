targets['build.o'] = Target(
    inputs=input_list(".cpp", ".o"),
    options = Options(
        appenders = dict(
            compiler_flags = [ '`wx-config --cxxflags`' ],
            compiler_defines = dict(
                WINVER='0x0400',
                __GNUWIN32__=None,
                __WIN95__=None,
                STRICT=None,
                HAVE_W32API_H=None,
                __WXMSW__=None,
                __WINDOWS__=None,
            ),
        )
    )
)
