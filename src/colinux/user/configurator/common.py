import wx
from common import *

class InvalidArrayIndexChosen(Exception):
    pass

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

    def __init__(self, splitter, mainframe, config_item_list=None):
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
        wx.EVT_BUTTON(edit, -1, self.edit)
        wx.EVT_BUTTON(remove, -1, self.remove)

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
            index = listctl.InsertStringItem(index, item.title(long=True))

        self.panel.SetSizer(panel_sizer)
        self.config_item_list = config_item_list
        self.listctl = listctl

    def add(self, event):
        xml = self.add_wizard(self.mainframe)
        if xml:
            self.config_item_list.populate_index(xml)
            self.config_item_list.add_item(xml)

    def remove(self, event):
        for index in xrange(self.listctl.GetItemCount()):
            if self.listctl.GetItemState(index, wx.LIST_STATE_SELECTED):
                self.get_item_list().del_item(index)
                self.listctl.DeleteItem(index)
                return

    def edit(self, event):
        for index in xrange(self.listctl.GetItemCount()):
            if self.listctl.GetItemState(index, wx.LIST_STATE_SELECTED):
                self.get_item_list().select_item_by_index(index)
                return

    class ItemPanel(OptionPanel):
        def __init__(self, item, title_changed, check_index, splitter, mainframe):
            super(OptionArrayPanel.ItemPanel, self).__init__(splitter, mainframe)

            label = wx.StaticText(self.panel, -1, item.title(), style=wx.ALIGN_CENTRE)
            panelSizer = wx.BoxSizer(wx.VERTICAL)
            panelSizer.Add(label, 0, wx.EXPAND | wx.ALL, 2)
            self.item = item
            self.populate(panelSizer)
            button = wx.Button(self.panel, 0,"Apply")
            panelSizer.Add(wx.Panel(self.panel, 0), 1,
                           wx.ALL | wx.ALIGN_RIGHT, 3)
            wx.EVT_BUTTON(button, 0, self.apply)
            panelSizer.Add(button, 0, wx.ALL | wx.ALIGN_RIGHT | wx.EXPAND, 10)
            button.Disable();
            self.apply_button = button;
            self.panel.SetSizer(panelSizer)
            self.title_changed = title_changed
            self.check_index = check_index

        def populate(self, panel_sizer):
            pass

        def apply(self, event=None):
            self.apply_button.Disable()
            self.title_changed(self.item.title())

        def changed(self, event=None):
            self.apply_button.Enable(True)

__all__ = ['OptionArrayPanel', 'OptionPanel', 'ConfigurationItem',
           'InvalidArrayIndexChosen']
