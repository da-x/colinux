#!/usr/bin/env python

import os, sys, time

def main_by_string(input_string):
    version = input_string.strip()
    output_lines = []
    output_lines.append('#ifndef __COLINUX_VERSION_H__')
    output_lines.append('#define __COLINUX_VERSION_H__')
    output_lines.append('')
    output_lines.append('#define COLINUX_VERSION      "%s"' % (version, ))
    output_lines.append('#define COLINUX_COMPILE_TIME "%s"' % (time.ctime(), ))
    output_lines.append('')
    output_lines.append('#endif')
    output_lines.append('')
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
