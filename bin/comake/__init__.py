import os

DEFAULT_BUILD_NAME = 'build.comake.py'
COMAKE_OUTPUT_DIRECTORY = '.comake.build'

def _build_root():
    from os.path import dirname
    return dirname(dirname(dirname(__file__)))

build_root = _build_root()
 
def main(args):
    filename = args[0]
    os.chdir(build_root)

    if args[0] == 'clean':
        from comake.target import clean
        clean()
    else:
        from comake.target import create_target_tree, statistics
        from comake.target import TargetNotFoundError
        print "Analyzing target tree..."
        try:
            target_tree = create_target_tree(filename)
        except TargetNotFoundError:
            return
            
        if '--dump' in args:
            target_tree.dump()
        else:
            print "Starting build"
            target_tree.build()
            if statistics.made_targets == 0:
                print "No targets were rebuilt."
            print "Total number of targets: %d" % (statistics.targets, )
            if statistics.made_targets != 0:
                print "Targets rebuilt: %d" % (statistics.made_targets, )
        
