import os
import wx
from wxPython import wizard as wxWizard
from xmlwrapper import XMLWrapper

class MainEditor(wx.SplitterWindow):
    def __init__(self, mainframe, *arg, **kw):
        wx.SplitterWindow.__init__(self, mainframe, *arg, **kw)
        
        tree = wx.TreeCtrl(self, -1)
        self.panel = wx.Panel(self, -1)

        items = []
        item = item_root = tree.AddRoot('Configuration')
        items.append(item)
        
        def append_item(parent, name, data=None):
            item = tree.AppendItem(parent, name)
            tree.SetPyData(item, data)
            items.append(item)
            return item
            
        self.item_devices = append_item(item_root, 'Devices')
        append_item(item_root, 'Kernel image')
        append_item(item_root, 'Memory')
        append_item(item_root, 'Boot parameters')

        for item in items:
            tree.Expand(item)

        def sel_changed(event):
            clss = tree.GetPyData(event.GetItem())
            if not clss:
                clss = OptionPanel
            instance = clss(self, mainframe)
            self.ReplaceWindow(self.panel, instance.panel)
            self.panel.Destroy()
            self.panel = instance.panel
            
        wx.EVT_TREE_SEL_CHANGED(tree, tree.GetId(), sel_changed)
        
        self.SplitVertically(tree, self.panel, 300)
        self.tree = tree

class ConfigurationItem(object):
    def __init__(self, xml_item):
        self._xml_item = xml_item

    def title(self):
        raise NotImplemented()
   
class OptionPanel(object):
    def __init__(self, splitter, mainframe):
        self.panel = wx.Panel(splitter, -1)

class OptionArrayPanel(OptionPanel):
    CLASS_DESC = None
    LONG_DESC = ''
    SHORT_TREE_DESC = None
    
    def __init__(self, splitter, mainframe):
        super(OptionArrayPanel, self).__init__(splitter, mainframe)
        self.mainframe = mainframe
        
        label = wx.StaticText(self.panel, -1, self.CLASS_DESC, style=wx.ALIGN_CENTRE)
        desc = wx.StaticText(self.panel, -1, self.LONG_DESC)
        listctl = wx.ListCtrl(self.panel, -1, style=wx.LC_LIST)
        listctl.InsertColumn(0, 'Title')

        add = wx.Button(self.panel, -1, 'Add')
        edit = wx.Button(self.panel, -1, 'Edit')
        remove = wx.Button(self.panel, -1, 'Remove')

        wx.EVT_BUTTON(add, -1, self.add)
        
        button_sizer = wx.BoxSizer(wx.HORIZONTAL)
        button_sizer.Add(add, 0, wx.ALL, 4)
        button_sizer.Add(edit, 0, wx.ALL, 4)
        button_sizer.Add(remove, 0, wx.ALL, 4)

        panel_sizer = wx.BoxSizer(wx.VERTICAL)
        panel_sizer.Add(label, 0, wx.EXPAND | wx.ALL, 2)
        panel_sizer.Add(desc, 0, wx.EXPAND | wx.ALL, 2)
        panel_sizer.Add(listctl, 1, wx.EXPAND | wx.ALL, 4)
        panel_sizer.Add(button_sizer, 0, wx.ALIGN_CENTRE)

        for index, item in enumerate(self.get_item_list().list):
            listctl.InsertStringItem(index, item.title(long=True))
        
        self.panel.SetSizer(panel_sizer)

    def add(self, event):
        xml = self.add_wizard(self.mainframe)

    class ItemPanel(OptionPanel):
        def __init__(self, item, splitter, mainframe):
            super(OptionArrayPanel.ItemPanel, self).__init__(splitter, mainframe)

            label = wx.StaticText(self.panel, -1, item.title(), style=wx.ALIGN_CENTRE)
            panelSizer = wx.BoxSizer(wx.VERTICAL)
            panelSizer.Add(label, 0, wx.EXPAND | wx.ALL, 2)
            self.panel.SetSizer(panelSizer)

class BlockDevicesOptionArray(OptionArrayPanel):
    CLASS_DESC = 'Storage Devices'
    SHORT_TREE_DESC = 'Storage'
    LONG_DESC = """Storage devices are represented in Linux as /dev/cobdX, where
X is the number identifying the storage device."""
    
    class Item(ConfigurationItem):
        DOS_DEVICES = '\\DosDevices\\'
        def title(self, long=False):
            index = int(self._xml_item.attr.index)
            if long:
                name = '/dev/cobd%d' % (index, )
            else:
                name = '[%d]' % (index, )
            path = self._xml_item.attr.path
            if path.startswith(self.DOS_DEVICES):
                title = str(path[len(self.DOS_DEVICES):])
            else:
                title = path
            title = '%s: %s' % (name, title)
            return title

    class ItemPanel(OptionArrayPanel.ItemPanel):
        pass

    def get_item_list(self):
        return self.mainframe.block_devices

    def add_wizard(self, mainframe):
        from blockdevice import run_wizard
        run_wizard(mainframe)

class NetworkDevicesOptionArray(OptionArrayPanel):
    CLASS_DESC = 'Virtual Network Adapters'
    SHORT_TREE_DESC = 'Networking'

    def get_item_list(self):
        return self.mainframe.network_devices

    class Item(ConfigurationItem):
        def title(self, long=False):
            device = self._xml_item
            index = int(device.attr.index)
            if long:
                name = 'conet%d' % (index, )
            else:
                name = '[%d]' % (index, )
            return '%s: %s' % (name, device.attr.type)

    class ItemPanel(OptionArrayPanel.ItemPanel):
        pass

class ConfigurationItemList(object):
    def __init__(self, optionpanel, xmlelements, maineditor):        
        self.root = maineditor.item_devices
        self.itemtree = maineditor.tree.AppendItem(self.root, optionpanel.SHORT_TREE_DESC)
        self.list = []
        self.optionpanel = optionpanel
        self.maineditor = maineditor        
        for item in xmlelements:
            self.add_item(item)

    def add_item(self, xml_item):
        instance = self.optionpanel.Item(xml_item)        
        listitem = self.maineditor.tree.AppendItem(self.itemtree, instance.title())
        self.maineditor.tree.Expand(self.itemtree)
        self.maineditor.tree.SetPyData(listitem, lambda *arg:self.optionpanel.ItemPanel(instance, *arg))
        self.list.append(instance)
        self.maineditor.tree.Expand(self.root)
        self.maineditor.tree.SetPyData(self.itemtree, self.optionpanel)

class MainFrame(wx.Frame):
    def __init__(self, configurator, parent, id, title):
        wx.Frame.__init__(self, parent, id, title, size=wx.Size(720, 400))

        self._main_editor = None
        self._modified = False

        class filedropclass(wx.FileDropTarget):
            def OnDropFiles(fdself, x, y, filenames):
                for filename in filenames:
                    self.gui_open(filename)
                    pass

        self.SetDropTarget(filedropclass())
        
        bar = self.CreateStatusBar(2)
        bar.SetStatusWidths([-1, 40])

        menuBar = wx.MenuBar()
        self.menuBar = menuBar
        menu = wx.Menu()
        menu.Append(wx.ID_NEW, '&New', 'New')
        wx.EVT_MENU(self, wx.ID_NEW, self.on_new)
        menu.Append(wx.ID_CLOSE, '&Close', 'Close')
        wx.EVT_MENU(self, wx.ID_CLOSE, self.on_close)
        menu.Append(wx.ID_OPEN, '&Open', 'Open a new configuration file')
        wx.EVT_MENU(self, wx.ID_OPEN, self.on_open)
        menu.Append(wx.ID_SAVE, '&Save', 'Save a configuration file')
        menu.Append(wx.ID_SAVEAS, 'Save &As...', 'Save a configuration file under different name')
        wizard_id = wx.NewId()
        menu.Append(wizard_id, '&Wizard', 'Wizard')
        menu.AppendSeparator()
        menu.Append(wx.ID_EXIT, '&Quit', 'Quit')
        menuBar.Append(menu, '&File')
        self.SetMenuBar(menuBar)
        
        wx.EVT_MENU(self, wx.ID_EXIT, self.on_close_window)

    def on_close_window(self, event):        
        self.Destroy()
        
    def AskSave(self):
        if not (self._modified): return True
        
        flags = wx.ICON_EXCLAMATION | wx.YES_NO | wx.CANCEL | wx.CENTRE
        dlg = wx.MessageDialog(self, 'File is modified. Save before exit?',
                                 'Save before too late?', flags )
        say = dlg.ShowModal()
        dlg.Destroy()
        if say == wx.ID_YES:
            #self.OnSaveOrSaveAs(wxCommandEvent(wxID_SAVE))
            if not self.modified: return True
        elif say == wx.ID_NO:
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
            dlg = wx.FileDialog(self, 'Open', '',
                                  '', '*.colinux.xml', wx.OPEN | wx.CHANGE_DIR)
            if dlg.ShowModal() == wx.ID_OK:
                path = dlg.GetPath()
            dlg.Destroy()

        if path is not None:
            self.open(path)

    def close(self):
        if self._main_editor:
            self._main_editor.Destroy()
            self._main_editor = None

    def new_layout(self):
        self.close()
        splitter = self.splitter = MainEditor(self, 2)
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(splitter, 1, wx.EXPAND)
        self.SetSizer(sizer)
        self.SetAutoLayout(True)
        self._main_editor = splitter
        self.SendSizeEvent()

    def new(self):
        self.new_layout()
        self.open_xml(XMLWrapper.parse_string('<colinux></colinux>'))

    def open(self, path):
        self.new_layout()
        self.open_xml(XMLWrapper.parse(open(path)))
        self.SetStatusText('Loaded: %s' % (path, ))

    def open_xml(self, xml):
        self.xml = xml
        self.xml_colinux, = self.xml.sub.colinux

        self.block_devices = ConfigurationItemList(
            BlockDevicesOptionArray,
            self.xml_colinux.sub.block_device,
            self._main_editor)

        self.network_devices = ConfigurationItemList(
            NetworkDevicesOptionArray,
            self.xml_colinux.sub.network,
            self._main_editor)
        
        self._modified = False        

class App(wx.App):
    def __init__(self, configurator, *arg, **kw):
        self._configurator = configurator
        wx.App.__init__(self, *arg, **kw)
        
    def OnInit(self):
        frame = MainFrame(self._configurator, None, -1, 'Cooperative Linux Virtual Machine Configuration')
        frame.Show(True)
        self.SetTopWindow(frame)
        self.mainframe = frame
        return True

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
