/*
 * This source code is a part of coLinux source package.
 *
 * Shai Vaingast, <svaingast at gmail.com >
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
#ifndef __COLINUX_USER_FONT_SELECT_H__
#define __COLINUX_USER_FONT_SELECT_H__
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Hold_Browser.H>

class FontDisplay : public Fl_Widget 
{
	void draw();
public:
	bool is_fixed_pitch(Fl_Font test_font, int test_size);
	FontDisplay(Fl_Boxtype B, int X, int Y, int W, int H, const char* L = 0):
		Fl_Widget(X,Y,W,H,L)  {box(B); font = 0; size = 14;};
	int get_size(void) { return size; }
	int get_font(void) { return font; }
	void set_font(int f) { font = f; }
	void set_size(int s) { size = s; }
private:
	int font, size;
};

class FontSelectDialog : public Fl_Window
{
public:
	FontSelectDialog(void*ptr);
	void click_font(void);
	void click_size(void);
	void *ptr_console;
	FontDisplay *textobj;
	
private:
	void populate_fonts(void);
	void init_display_string(void);

	Fl_Hold_Browser *sizeobj;
	Fl_Hold_Browser *fontobj;
	int pickedsize;
	int **my_sizes;
	int *numsizes;
	char str_label[0x200];
	
};

#endif
