# This is not a standalone Python script, but a build declaration file
# to be read by bin/make.py. Please run bin/make.py --help.

targets['colinux'] = Target(
    inputs=[Input('src/build')],
    tool = Empty(),
)

