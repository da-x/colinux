import Tix

class MainWindow(object):
    def __init__(self):
        self._root = root = Tix.Tk()
#         self._menu = menu = Tix.Menu(self._root)
#         menu.add_command(label="File", command=self.file)
#         menu.add_command(label="File", command=self.file)
#         menu.add_command(label="File", command=self.file)
#         menu.add_command(label="File", command=self.file)
#         menu.add_command(label="File", command=self.file)
#         root.config(menu=menu)

        root.geometry("640x400")

        container = Tix.Frame(root)
        container.pack(padx=2, pady=2, expand=Tix.YES, fill=Tix.BOTH)
        
        container = Tix.Frame(container)
        container.pack(fill=Tix.BOTH, side=Tix.LEFT, expand=Tix.YES,  padx=2, pady=2, ipadx=2, ipady=2)
        
        self.tree = tree = Tix.ScrolledHList(container, width=200)
        tree.pack(expand=Tix.YES, side=Tix.LEFT, fill=Tix.BOTH)

        hlist = tree.hlist
        hlist.config(separator='.', width=25, drawbranch=1, indent=10)
        hlist.add('disks', itemtype=Tix.TEXT, text='Disks (block devices)')
        hlist.add('disks.add', itemtype=Tix.TEXT, text='Add...')
        hlist.add('network', itemtype=Tix.TEXT, text='Virtual Network')
        hlist.add('network.add', itemtype=Tix.TEXT, text='Add...')
        hlist.add('memory', itemtype=Tix.TEXT, text='Memory')
        hlist.add('misc', itemtype=Tix.TEXT, text='Miscellaneous')

        hlist["background"] = "white"
        print hlist.event_info()
        #tree.browsecmd(self.select)
        hlist.bind("<Button-3>", self.select)
        
        settings = Tix.Frame(container, width=500)
        settings.pack(side=Tix.RIGHT, padx=2, pady=2, ipadx=2, ipady=2)

    def select(self, event):
        hlist = self.tree.hlist
        hlist.delete_entry('disks.add')
        try:
            import time, random
            hlist.add('disks.xad' + chr(ord('a') + random.randrange(30)), itemtype=Tix.TEXT, text='Bla')
        except:
            pass
        hlist.add('disks.add', itemtype=Tix.TEXT, text='Add...')

    def run(self):
        self._root.mainloop()

def main_args(args):
    MainWindow().run()

def main():
    from sys import argv, exit
    exit(main_args(argv))

if __name__ == '__main__':
    main()
