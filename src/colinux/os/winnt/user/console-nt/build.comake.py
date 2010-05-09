targets['build.a'] = Target(
    inputs=input_list('.cpp', '.o')
          +input_list('.c', '.o'),
)
