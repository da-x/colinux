#!/usr/bin/env python

import sys

def main():
    from comake import main as comake_main
    comake_main(sys.argv[1:])

if __name__ == "__main__":
    main()
