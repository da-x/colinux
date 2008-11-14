
#ifndef __WXCONSOLE_H
#define __WXCONSOLE_H

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
        #include "wx/wx.h"
#endif
#include "wx/imaglist.h"
#include "wx/artprov.h"
#include "wx/cshelp.h"
#include "wx/utils.h"

#include <colinux/user/console-base/console.h>

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

class console_window_WX_t : public console_window_t {
public:
	console_window_WX_t(MyFrame *frame) { m_frame = frame; }
        virtual const char *get_name();
private:
	MyFrame *m_frame;
};

#endif
