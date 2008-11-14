
extern "C" {
#include <colinux/common/debug.h>
#include <colinux/os/user/misc.h>
}

#include <colinux/user/console-base/main.h>
#include "console.h"

COLINUX_DEFINE_MODULE("colinux-console-wx");

#if 0
int co_console_init(int argc, char **argv,class MyFrame *frame)
{
	int status;

	global_window = new console_window_WX_t(frame);

	if (!global_window) {
		co_terminal_print("Unable to create console window");
		return -1; 
	}

	try {
		status = co_user_console_main(argc, argv);
	} catch(...) {
		co_debug("The console program encountered an exception.");
		status = -1;
	}

	delete global_window;

	return status;
}
#endif

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	if ( !wxApp::OnInit() ) return false;
	MyFrame *frame = new MyFrame(_T("coLinux Console"));
	frame->Show(true);

//	co_console_init(argc,argv,frame);
	return true;
}

#if 0
int co_user_console_main(int argc, char **argv)
{
        co_rc_t rc;
        console_window_t window;
        int ret;

        global_window = &window;

        co_debug_start();

        rc = window.parse_args(argc, argv);
        if (!CO_OK(rc)) {
                co_debug("Error %08x parsing parameters", (int)rc);
                ret = -1;
                goto out;
        }

        rc = window.start();
        if (!CO_OK(rc)) {
                co_debug("Error %08x starting console", (int)rc);
                ret = -1;
                goto out;
        }

        ret = Fl::run();
out:
        co_debug_end();

        return ret;
}
#endif
