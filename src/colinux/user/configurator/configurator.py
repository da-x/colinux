import os
from wxPython import wx
from xml.dom import minidom

class MainEditor(wx.wxSplitterWindow):
    def __init__(self, *arg, **kw):
        wx.wxSplitterWindow.__init__(self, *arg, **kw)

        tree = wx.wxTreeCtrl(self, -1)
        panel = wx.wxPanel(self, -1)

        items = []
        item = item_root = tree.AddRoot('coLinux Configuration')
        items.append(item)
        item = item_devices = tree.AppendItem(item_root, 'Devices')
        items.append(item)
        self.block_devices = item = tree.AppendItem(item_devices, 'Disks')
        items.append(item)
        self.network_devices =item = tree.AppendItem(item_devices, 'Network')
        items.append(item)
        item = tree.AppendItem(item_root, 'Kernel image')
        items.append(item)
        item = tree.AppendItem(item_root, 'Memory')
        items.append(item)
        item = tree.AppendItem(item_root, 'Boot parameters')
        items.append(item)
        for item in items:
            tree.Expand(item)
        
        self.SplitVertically(tree, panel, 300)
        self.tree = tree

class ConfigurationItem(object):
    def __init__(self, xml_item, tree, parent):
        self._xml_item = xml_item
        tree.AppendItem(parent,  self.title())
        tree.Expand(parent)

    def title(self):
        raise NotImplemented()

class BlockDeviceConfigurationItem(ConfigurationItem):
    DOS_DEVICES = '\\DosDevices\\'
    def title(self):
        index = int(self._xml_item.getAttribute('index'))
        name = "cobd%d" % (index, )
        path = self._xml_item.getAttribute('path')
        if path.startswith(self.DOS_DEVICES):
            title = 'File: %s' % (path[len(self.DOS_DEVICES):], )
        else:
            title = path
        title = '%s: %s' % (name, title)
        return title

class NetworkDeviceConfigurationItem(ConfigurationItem):
    def title(self):
        device = self._xml_item
        return "conet%d: %s" % (int(device.getAttribute('index')), device.getAttribute('type'))
    
class MainFrame(wx.wxFrame):
    def __init__(self, configurator, parent, id, title):
        wx.wxFrame.__init__(self, parent, id, title, size=wx.wxSize(720, 400))

        self._main_editor = None
        self._modified = False

        class filedropclass(wx.wxFileDropTarget):
            def OnDropFiles(fdself, x, y, filenames):
                for filename in filenames:
                    self.gui_open(filename)
                    pass

        self.SetDropTarget(filedropclass())
        
        bar = self.CreateStatusBar(2)
        bar.SetStatusWidths([-1, 40])

        menuBar = wx.wxMenuBar()
        self.menuBar = menuBar
        menu = wx.wxMenu()
        menu.Append(wx.wxID_NEW, '&New', 'New')
        wx.EVT_MENU(self, wx.wxID_NEW, self.on_new)
        menu.Append(wx.wxID_CLOSE, '&Close', 'Close')
        wx.EVT_MENU(self, wx.wxID_CLOSE, self.on_close)
        menu.Append(wx.wxID_OPEN, '&Open', 'Open a new configuration file')
        wx.EVT_MENU(self, wx.wxID_OPEN, self.on_open)
        menu.Append(wx.wxID_SAVE, '&Save', 'Save a configuration file')
        menu.Append(wx.wxID_SAVEAS, 'Save &As...', 'Save a configuration file under different name')
        wizard_id = wx.wxNewId()
        menu.Append(wizard_id, '&Wizard', 'Wizard')
        menu.AppendSeparator()
        menu.Append(wx.wxID_EXIT, '&Quit', 'Quit')
        menuBar.Append(menu, '&File')
        self.SetMenuBar(menuBar)
        
        wx.EVT_MENU(self, wx.wxID_EXIT, self.on_close_window)

    def on_close_window(self, event):        
        self.Destroy()
        
    def AskSave(self):
        if not (self._modified): return True
        
        flags = wx.wxICON_EXCLAMATION | wx.wxYES_NO | wx.wxCANCEL | wx.wxCENTRE
        dlg = wx.wxMessageDialog( self, 'File is modified. Save before exit?',
                               'Save before too late?', flags )
        say = dlg.ShowModal()
        dlg.Destroy()
        if say == wx.wxID_YES:
            #self.OnSaveOrSaveAs(wxCommandEvent(wxID_SAVE))
            if not self.modified: return True
        elif say == wx.wxID_NO:
            return True
        return False

    def on_open(self, event):
        self.gui_open()

    def on_new(self, path):
        if not self.AskSave(): return
        self.new()

    def on_close(self, path):
        if not self.AskSave(): return
        self.close()

    def gui_open(self, path=None):
        if not self.AskSave(): return
        
        if path is None:
            dlg = wx.wxFileDialog(self, 'Open', '',
                                  '', '*.colinux.xml', wx.wxOPEN | wx.wxCHANGE_DIR)
            if dlg.ShowModal() == wx.wxID_OK:
                path = dlg.GetPath()
                
            dlg.Destroy()

        if path is not None:
            self.open(path)

    def close(self):
        if self._main_editor:
            self._main_editor.Destroy()
            self._main_editor = None

    def new(self):
        self.close()
        self._block_devices = []
        self._network_devices = []
        splitter = self.splitter = MainEditor(self, 2)
        sizer = wx.wxBoxSizer(wx.wxVERTICAL)
        sizer.Add(splitter, 1, wx.wxEXPAND)        
        self.SetSizer(sizer)
        self.SetAutoLayout(wx.true)
        self._main_editor = splitter
        self.SendSizeEvent()

    def open(self, path):
        self.new()
        self.xml = xml = minidom.parse(open(path))
        self.xml_colinux = None
        
        for element in xml.childNodes:
            if element.nodeName == 'colinux':
                self.xml_colinux = element
                for element in element.childNodes:
                    if element.nodeName == 'block_device':
                        parent = self._main_editor.block_devices
                        confclass = BlockDeviceConfigurationItem
                        itemlist = self._block_devices
                    elif element.nodeName == 'network':
                        parent = self._main_editor.network_devices
                        confclass = NetworkDeviceConfigurationItem
                        itemlist = self._network_devices
                    else:
                        continue
                        
                    itemlist.append(confclass(element, self._main_editor.tree, parent))

        self._modified = False
        self.SetStatusText('Loaded: %s' % (path, ))

class App(wx.wxApp):
    def __init__(self, configurator, *arg, **kw):
        self._configurator = configurator
        wx.wxApp.__init__(self, *arg, **kw)
        
    def OnInit(self):
        frame = MainFrame(self._configurator, wx.NULL, -1, "coLinux configuration")
        frame.Show(wx.true)
        self.SetTopWindow(frame)
        self.mainframe = frame
        return wx.true

class Configurator(object):
    def __init__(self, args):
        self._app = App(self, 0)
        initial_pathname = None
        if args[1:]:
            initial_pathname = args[1]
        if initial_pathname:
            self.load_configuration(initial_pathname)

    def load_configuration(self, pathname):
        self._app.mainframe.open(pathname)

    def run(self):
        self._app.MainLoop()

def main_args(args):
    Configurator(args).run()

def main():
    from sys import argv, exit
    exit(main_args(argv))

if __name__ == '__main__':
    main()
