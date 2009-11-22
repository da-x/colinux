
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
	#include "wx/wx.h"
#endif
#include "wx/imaglist.h"
#include "wx/artprov.h"
#include "wx/cshelp.h"
#include "wx/utils.h"
#include "console.h"

extern "C" {
#include <colinux/common/ioctl.h>
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	if ( !wxApp::OnInit() ) return false;
	MyFrame *frame = new MyFrame(_T("coLinux Console"));
	frame->Show(true);
	return true;
}

MyFrame::MyFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title)
{
	SetIcon(wxICON(colinux));

	wxMenu* fileMenu = new wxMenu;
	fileMenu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit this program"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(wxID_ABOUT, _T("&About...\tF1"), _T("Show about dialog"));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, _T("&File"));
	menuBar->Append(helpMenu, _T("&Help"));
	SetMenuBar(menuBar);

	CreateStatusBar(2);
	SetStatusText(_T("Not connected"),1);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
END_EVENT_TABLE()

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxString::Format(
                    _T("Copyright(C) 2008 Steve Shoecraft\n\n")
                    _T("This program is licensed under the GPL. See the COPYING file\n")
		    _T("that came with your distribution for more information.\n")
                 ),
                 _T("About coLinux Console"),
                 wxOK | wxICON_INFORMATION,
                 this);
}
