#!/usr/bin/env python

import sys, os, re

# 
# GCC wrapper for auto tracing
#
# It works using these steps:
#  * Take a regular GCC command line
#  * Execute this command line but with a modification that causes
#    is to output a processed C file instead.
#  * Execute autotrace.py to add traces to the preprocessed output.
#  * Compile the preprocessed output with the added traces.
# 

def reexec(params):
    pid = os.fork()
    if pid == 0:
        try:
            os.execvp(params[0], params);
        finally:
            os._exit(-1);
        
    (pid, status) = os.waitpid(pid, 0)
    if not os.WIFEXITED(status):
        sys.exit(-1)
        
    return os.WEXITSTATUS(status)

params = sys.argv[1:]
if ('-c' not in params) or (not (os.getenv("GCCTRACE") == 'Y')):
    sys.exit(reexec(params))

c_file = None
for param in params:
    if re.match(r'^(.+)?[.]c$', param):
        c_file = param

if '-o' not in params:
    params.append('-o')
    params.append(os.path.splitext(c_file)[0]+'.o')

if c_file == None or c_file == "init_task.c":
    sys.exit(reexec(params))

params.append('-DCOLINUX_TRACE')

ppc_params = params[:]
ppc_params.remove('-c')
ppc_params.append('-E')
    
oparam = ppc_params.index('-o')

ppc_file = ppc_params[oparam+1] = ppc_params[oparam+1] + '.ppc.c'
trace_file = ppc_params[oparam+1] + '.trace.c'

reexec(ppc_params)

import autotrace

autotrace.CTracer(open(ppc_file).read(), open(trace_file, 'w')).run()

params.remove(c_file)
params.append(trace_file)

result = reexec(params)

os.unlink(ppc_file)
#os.unlink(trace_file)

sys.exit(result)
