#!/usr/bin/env python

import os, sys, time, re

def main_by_string(input_string):
    version = input_string.strip()
    m = re.match("([^-]+)-pre[0-9]", version)
    if m:
        version = m.groups(0)[0]
    output_lines = []
    output_lines.append('!define VERSION "%s"' % (version, ));
    output_lines.append('!define LONGVERSION "%s.1"' % (version, ));
    return '\n'.join(output_lines)

def main_by_params(inputf, outputf):
    input_data = open(inputf, 'r').read()
    output_data = main_by_string(input_data)
    open(outputf, 'w').write(output_data)

def main_by_strparams(params):
    inputf, outputf = params[1:3]
    return main_by_params(inputf, outputf)

def main():
    import sys
    return main_by_strparams(sys.argv)

if __name__ == '__main__':
    main()