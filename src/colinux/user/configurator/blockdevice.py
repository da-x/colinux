import wx
from wxPython import wizard as wxWizard

def run_wizard(mainframe):    
    class PrevLinkedPage(wxWizard.wxPyWizardPage):
        def __init__(self, prev, wizard, *arg, **kw):
            wxWizard.wxPyWizardPage.__init__(self, wizard, *arg, **kw)
            self.wizard = wizard
            self.prev = prev
            self.populate()

        def populate(self):
            pass
        
        def GetPrev(self):
            return self.prev

    class RegularExistingFile(PrevLinkedPage):
        def populate(self):
            sizer = wx.BoxSizer(wx.VERTICAL)
            text = wx.StaticText(self, -1, 'Filename: ')
            sizer.Add(text, 0, wx.EXPAND | wx.ALL, 3)
            self.text = textctrl = wx.TextCtrl(self, -1, '')
            sizer.Add(textctrl, 0, wx.EXPAND | wx.ALL, 3)
            button = wx.Button(self, -1, "Browse")
            wx.EVT_BUTTON(button, -1, self.browse)
            sizer.Add(button, 0, wx.EXPAND | wx.ALL, 3)
            self.SetSizer(sizer)

        def browse(self, event):
            dlg = wx.FileDialog(self, 'Open', '',
                                '', '*', wx.OPEN | wx.CHANGE_DIR)
            if dlg.ShowModal() == wx.ID_OK:
                path = dlg.GetPath()
                self.text.SetValue(path)
                dlg.Destroy()

    class NewSparseFile(PrevLinkedPage):
        pass

    class RegularFile(PrevLinkedPage):
        def populate(self):
            sizer = wx.BoxSizer(wx.VERTICAL)
            choices = ["Use an existing file",
                       "Create a new sparse file", ]
            radio = wx.RadioBox(self, -1, "File type",
                                 choices=choices, style = wx.RA_SPECIFY_COLS, majorDimension=1)
            sizer.Add(radio, 0, wx.EXPAND | wx.ALL, 3)
            self.SetSizer(sizer)
            self.radio = radio

        def GetNext(self):
            next_classes = [
                RegularExistingFile,
                NewSparseFile,
            ]
            return next_classes[self.radio.GetSelection()](self, self.wizard)

    class ExistingDevice(wxWizard.wxPyWizardPage):
        def __init__(self, prev, wizard, *arg, **kw):
            wxWizard.wxPyWizardPage.__init__(self, wizard, *arg, **kw)
            self.prev = prev

        def GetPrev(self):
            return self.prev

    class FirstPage(wxWizard.wxPyWizardPage):
        def __init__(self, wizard, *arg, **kw):
            wxWizard.wxPyWizardPage.__init__(self, wizard, *arg, **kw)

            sizer = wx.BoxSizer(wx.VERTICAL)
            text = wx.StaticText(self, -1,
                                 "Please specify which kind of virtual disk you would like to add.")

            sizer.Add(text, 0, wx.EXPAND | wx.ALL, 1)
            choices = ["Raw file image",
                       "Use an existing partition or device", ]

            radio = wx.RadioBox(self, -1, "Virtual disk type",
                                 choices=choices, style = wx.RA_SPECIFY_COLS, majorDimension=1)

            text = wx.StaticText(self, -1, '\n'.join([
                "If you have a ready root file system image, you can use it",
                "by choosing '%s'." % (choices[0], ),
                "",
                "Otherwise, if you would like to use an already install Linux",
                "disk parition or storage device, choose '%s'." % (choices[1], ),
                '',
                ]))

            sizer.Add(text, 0, wx.EXPAND | wx.ALL, 1)
            sizer.Add(radio, 0, wx.EXPAND | wx.ALL, 1)
            sizer_center = wx.BoxSizer(wx.VERTICAL)
            sizer_center.Add(sizer, 0, wx.CENTER, 1)
            sizer = sizer_center
            self.SetSizer(sizer)
            self.wizard = wizard
            self.radio = radio

        def GetNext(self):
            next_classes = [
                RegularFile,
                ExistingDevice,
            ]
            return next_classes[self.radio.GetSelection()](self, self.wizard)

    wizard = wxWizard.wxWizard(mainframe, title="Add a new virtual disk")
    page = FirstPage(wizard)
    wizard.FitToPage(page)
    wizard.SetSize(wx.Size(200,100))
    wizard.RunWizard(page)


if __name__ == '__main__':
    run_wizard(wx.Frame(wx.NULL, -1, "test"))
