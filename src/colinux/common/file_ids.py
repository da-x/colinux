import sys

print "#include <stdlib.h>"
print "const char *colinux_obj_filenames[] = {"
for filename in sys.stdin.read().strip().split(' '):
    print '     "%s", ' % (filename, )
print ' NULL,'
print "};"

