import os
import wx
from wxPython import wizard as wxWizard
from xmlwrapper import XMLWrapper
from common import *
from blockdevice import BlockDevicesOptionArray
from networkdevice import NetworkDevicesOptionArray

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
        self.mainframe = mainframe

class ConfigurationItem(object):
    def __init__(self, xml_item):
        self._xml_item = xml_item

    def title(self):
        raise NotImplemented()

class ConfigurationItemList(object):
    def __init__(self, optionpanel, xmlelements, maineditor, index_range):
        self.root = maineditor.item_devices
        self.itemtree = maineditor.tree.AppendItem(self.root, optionpanel.SHORT_TREE_DESC)
        self.list = []
        self.optionpanel = optionpanel
        self.maineditor = maineditor
        self.elements = xmlelements
        for item in xmlelements:
            self.add_item(item, xml_update=False)
        self.maineditor.tree.SetPyData(self.itemtree, lambda *arg, **kw:optionpanel(config_item_list=self, *arg))
        self.index_range = index_range

    def used_indexes(self):
        return [int(item._xml_item.attr.index) for item in self.list]

    def populate_index(self, xml_item):
        used_index_list = self.used_indexes()
        for valid_index in self.index_range:
            if valid_index not in used_index_list:
                xml_item.attr.index = str(valid_index)
                return True
        return False

    def check_index(self, str_index, old_index):
        try:
            index = int(str_index)
        except ValueError, e:
            raise InvalidArrayIndexChosen("Index must be an integer", index)

        if int(old_index) != int(index):
            if index in self.used_indexes():
                raise InvalidArrayIndexChosen("Index already used", index)
            if index not in self.index_range:
                raise InvalidArrayIndexChosen("Index out of range", index)

        return str(index)

    def get_item_by_index(self, index):
        tree = self.maineditor.tree
        count = tree.GetChildrenCount(self.itemtree)
        cookie = 0
        child = ''
        for i in xrange(count):
            if i == 0:
                (child, cookie) = tree.GetFirstChild(self.itemtree, cookie)
            else:
                (child, cookie) = tree.GetNextChild(self.itemtree, cookie)
            if i == index:
                return child
        return None

    def select_item_by_index(self, index):
        tree = self.maineditor.tree
        item = self.get_item_by_index(index)
        tree.SelectItem(item)

    def del_item(self, index):
        tree = self.maineditor.tree
        item = self.get_item_by_index(index)
        if item:
            tree.Delete(item)
        del self.list[index]

    def add_item(self, xml_item, xml_update=True):
        instance = self.optionpanel.Item(xml_item)
        listitem = self.maineditor.tree.AppendItem(self.itemtree, instance.title())
        set_title_func = lambda title:self.maineditor.tree.SetItemText(listitem, title)
        self.maineditor.tree.Expand(self.itemtree)
        self.maineditor.tree.SetPyData(listitem, \
                                       lambda *arg:self.optionpanel.ItemPanel(instance,
                                                                              set_title_func,
                                                                              self.check_index, *arg))
        self.list.append(instance)
        self.maineditor.tree.Expand(self.root)
        if xml_update:
            self.elements.object.appendChild(xml_item._object)

class MainFrame(wx.Frame):
    def __init__(self, configurator, parent, id, title):
        wx.Frame.__init__(self, parent, id, title, size=wx.Size(720, 400))

        self._main_editor = None

        class filedropclass(wx.FileDropTarget):
            def OnDropFiles(fdself, x, y, filenames):
                for filename in filenames:
                    self.gui_open(filename)
                    pass

        self.SetDropTarget(filedropclass())
        self._current_path = None
        self._opened = False

        bar = self.CreateStatusBar(2)
        bar.SetStatusWidths([-1, 40])

        menuBar = wx.MenuBar()
        self.menuBar = menuBar
        menu = wx.Menu()
        menu.Append(wx.ID_NEW, '&New', 'New')
        wx.EVT_MENU(self, wx.ID_NEW, self.on_new)
        menu.Append(wx.ID_CLOSE, '&Close', 'Close')
        menu.Enable(wx.ID_CLOSE, False)
        wx.EVT_MENU(self, wx.ID_CLOSE, self.on_close)
        menu.Append(wx.ID_OPEN, '&Open', 'Open a new configuration file')
        wx.EVT_MENU(self, wx.ID_OPEN, self.on_open)
        menu.Append(wx.ID_SAVE, '&Save', 'Save a configuration file')
        wx.EVT_MENU(self, wx.ID_SAVE, self.on_save)
        menu.Enable(wx.ID_SAVE, False)
        menu.Append(wx.ID_SAVEAS, 'Save &As...', 'Save a configuration file under different name')
        wx.EVT_MENU(self, wx.ID_SAVEAS, self.on_save_as)
        menu.Enable(wx.ID_SAVEAS, False)
        wizard_id = wx.NewId()
        menu.Append(wizard_id, '&Wizard', 'Wizard')
        menu.AppendSeparator()
        menu.Append(wx.ID_EXIT, '&Quit', 'Quit')
        menuBar.Append(menu, '&File')
        self.SetMenuBar(menuBar)

        wx.EVT_MENU(self, wx.ID_EXIT, self.on_close_window)

    def on_close_window(self, event):
        self.ask_save()
        self.Destroy()

    def ask_save(self):
        if not self.is_modified(): return True

        flags = wx.ICON_EXCLAMATION | wx.YES_NO | wx.CANCEL | wx.CENTRE
        dlg = wx.MessageDialog(self, 'File is modified. Save before exit?',
                                 'Save before too late?', flags )
        say = dlg.ShowModal()
        dlg.Destroy()
        if say == wx.ID_YES:
            self.on_save(None)
            if not self.is_modified():
                return True
        elif say == wx.ID_NO:
            return True
        return False

    def on_open(self, event):
        self.gui_open()

    def on_new(self, event):
        if not self.ask_save(): return
        self.new()

    def on_close(self, event):
        if not self.ask_save(): return
        self.close()

    def on_save(self, event):
        if not self._current_path:
            return self.on_save_as(event)
        self.gui_save(self._current_path)

    def on_save_as(self, event):
        dlg = wx.FileDialog(self, 'Save', '',
                            '', '*.colinux.xml', wx.SAVE | wx.CHANGE_DIR)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.gui_save(path)
            dlg.Destroy()

    def gui_save(self, pathname):
        self.save(pathname)

    def gui_open(self, path=None):
        if not self.ask_save(): return

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
        self._opened = False
        self.menuBar.Enable(wx.ID_CLOSE, False)
        self.menuBar.Enable(wx.ID_SAVE, False)
        self.menuBar.Enable(wx.ID_SAVEAS, False)

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
        self._current_path = None

    def open(self, path):
        self.new_layout()
        wrapped = XMLWrapper.parse(open(path))
        colinux, = wrapped.sub.colinux
        colinux.striptext()
        self.open_xml(wrapped)
        self.SetStatusText('Loaded: %s' % (path, ))
        self._current_path = path

    def save(self, path):
        xml_to_save = self.xml.build_string()
        open(path, 'w').write(xml_to_save)
        self.xml_before_edit = xml_to_save
        self.SetStatusText('Saved: %s' % (path, ))
        self._current_path = path

    def open_xml(self, xml):
        self.xml = xml
        self.xml_colinux, = self.xml.sub.colinux
        self._opened = True
        self.menuBar.Enable(wx.ID_CLOSE, True)
        self.menuBar.Enable(wx.ID_SAVE, True)
        self.menuBar.Enable(wx.ID_SAVEAS, True)

        self.block_devices = ConfigurationItemList(
            BlockDevicesOptionArray,
            self.xml_colinux.sub.block_device,
            self._main_editor,
            range(32))

        self.network_devices = ConfigurationItemList(
            NetworkDevicesOptionArray,
            self.xml_colinux.sub.network,
            self._main_editor,
            range(32))

        self.xml_before_edit = self.xml.build_string()

    def is_modified(self):
        if not self._opened:
            return False
        return self.xml.build_string() != self.xml_before_edit

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
