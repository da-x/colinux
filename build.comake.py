# This is not a standalone Python script, but a build declaration file
# to be read by bin/make.py. Please run bin/make.py --help.

from comake.settings import settings

targets['colinux'] = Target(
    inputs = [Input('src/build')],
    tool = Empty(),
)

targets['installer'] = Target(
    inputs = [Input('colinux')],
    tool = Empty(),
    settings_options=Options(
        overriders=dict(
            final_build_target='installer',
        ),
    ),
)
