import sys, os

if sys.argv[1] == 'create':
    files_list = open(sys.argv[2]).read().split(' ')
    try:
        print files_list.index(sys.argv[3])
    except:
        print 0
elif sys.argv[1] == 'update':
    cmdline_data = ' '.join([filename[2:] for filename in sys.argv[3:]])

    if os.path.exists(sys.argv[2]):
        file_data = open(sys.argv[2]).read()
        if file_data != cmdline_data:
            open(sys.argv[2], 'w').write(cmdline_data)
    else:
        open(sys.argv[2], 'w').write(cmdline_data)
