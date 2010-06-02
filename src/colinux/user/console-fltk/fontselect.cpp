/*
 * This source code is a part of coLinux source package.
 *
 * Shai Vaingast, <svaingast at gmail.com >
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
 
#include "fontselect.h"
#include "console.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Button.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

// callback functions 
static void size_cb(Fl_Widget *widget, void*v)
{
	((FontSelectDialog*)v)->click_size();
	return;
}

static void font_cb(Fl_Widget *widget, void* v)
{
	((FontSelectDialog*)v)->click_font();
}

static void on_font_select(Fl_Widget *widget, void* v)
{
	// v points to this FontSelectDialog instance
	FontSelectDialog *fsd_inst=(FontSelectDialog *)v;
	
	// fsd_inst->ptr_console points to this console_window_t instance
	console_window_t* cw_inst = ((console_window_t*)(((Fl_Menu_Item*)fsd_inst->ptr_console)->user_data_));
	
	cw_inst->get_widget()->set_font_name(fsd_inst->textobj->get_font());
	cw_inst->get_widget()->set_font_size(fsd_inst->textobj->get_size());
	cw_inst->resize_font();
	// WriteRegistry(REGISTRY_FONT_SIZE, 36);
}

static void on_font_cancel(Fl_Widget *widget, void* v)
{
	((FontSelectDialog*)v)->hide();
}

FontSelectDialog::FontSelectDialog(void*ptr): Fl_Window(530, 370, "Font select") 
{  
	ptr_console = ptr;
	sizeobj = NULL;
	textobj = NULL;
	fontobj = NULL;
	pickedsize = 14;
	my_sizes = NULL;
	numsizes = NULL;
	strnset(str_label, 0, 0x200);
	
	init_display_string();

	// a canvas to display the fonts
	textobj = new FontDisplay(FL_FRAME_BOX, 10, 190, 390, 170, str_label);
	textobj->align(FL_ALIGN_TOP|FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);
	//textobj->color(9,47);

	// font types
	fontobj = new Fl_Hold_Browser(10, 10, 390, 170);
	fontobj->box(FL_FRAME_BOX);
	fontobj->color(53,3);
	fontobj->callback((Fl_Callback*)font_cb, this);
	resizable(fontobj);

	// font sizes
	sizeobj = new Fl_Hold_Browser(410, 10, 110, 170);
	sizeobj->box(FL_FRAME_BOX);
	sizeobj->color(53,3);
	sizeobj->callback((Fl_Callback*)size_cb, this);

	// cancel and select buttons
    Fl_Button * btn_select	= new Fl_Button(410, 290, 110,30, "Select" );
    btn_select->box(FL_FRAME_BOX);
	btn_select->callback((Fl_Callback*)on_font_select, this);
    Fl_Button * btn_cancel	= new Fl_Button(410, 330, 110,30, "Cancel" );
    btn_cancel->box(FL_FRAME_BOX);
	btn_cancel->callback((Fl_Callback*)on_font_cancel, this);
	
	// done with display
	end();

	populate_fonts();
}

void FontSelectDialog::populate_fonts(void)
{
	Fl::scheme(NULL);
	Fl::get_system_colors();
	
	int num_fonts = Fl::set_fonts(NULL);	
	my_sizes = new int*[num_fonts];
	numsizes = new int[num_fonts];
	for (int i = 0; i < num_fonts; i++) 
	{
		int t; 
		const char *name = Fl::get_font_name((Fl_Font)i,&t);
		char buffer[128];
		if (t) 
		{
			char *p = buffer;
			if (t & FL_BOLD) {*p++ = '@'; *p++ = 'b';}
			if (t & FL_ITALIC) {*p++ = '@'; *p++ = 'i';}
			strcpy(p,name);
			name = buffer;
		}
		fontobj->add(name);
		int *s; 
		int n = Fl::get_font_sizes((Fl_Font)i, s);
		numsizes[i] = n;
		if (n) 
		{
			my_sizes[i] = new int[n];
			int  j;
			for (j=0; j<n; j++) my_sizes[i][j] = s[j];
			
			// not working great, some issues here
			//if(textobj->is_fixed_pitch((Fl_Font)i, s[j]))
				
		}
	}
	fontobj->value(1);
	//font_cb(this, 0);
	click_font();
}

// FontSelectDialog functions
void FontSelectDialog::click_size(void)
{
	if((!sizeobj) || (!textobj))
		return;
	int i = sizeobj->value();
	if (!i)
		return;
	const char *c = sizeobj->text(i);
	while (*c < '0' || *c > '9') c++;
	pickedsize = atoi(c);
	textobj->set_size(pickedsize);
	textobj->redraw();
}

void FontSelectDialog::click_font(void)
{
	if((!sizeobj) || (!textobj))
		return;
	int fn = fontobj->value();
	if (!fn)
		return;
	fn--;
	textobj->set_font(fn);
	sizeobj->clear();

	int n = numsizes[fn];
	int *s = my_sizes[fn];
	if (!n) 
	{
		// no my_sizes
	} 
	else if (s[0] == 0) 
	{
		// many my_sizes;
		int j = 1;
		for (int i = 1; i<64 || i<s[n-1]; i++) 
		{
			char buf[20];
			if (j < n && i==s[j]) 
			{
				sprintf(buf,"@b%d",i); 
				j++;
			}
			else 
				sprintf(buf,"%d",i);
			sizeobj->add(buf);
		}
		sizeobj->value(pickedsize);
	} 
	else 
	{
		// some my_sizes
		int w = 0;
		for (int i = 0; i < n; i++) 
		{
			if (s[i]<=pickedsize) w = i;
			char buf[20];
			sprintf(buf,"@b%d",s[i]);
			sizeobj->add(buf);
		}
		sizeobj->value(w+1);
	}
	textobj->redraw();
}

void FontDisplay::draw() 
{
	draw_box();
	fl_font((Fl_Font)font, size);
	fl_color(FL_BLACK);
	fl_draw(label(), x()+3, y()+3, w()-6, h()-6, align());
}

bool FontDisplay::is_fixed_pitch(Fl_Font test_font, int test_size)
{
	fl_font(test_font, test_size);
	if ((int)fl_width('i') != (int)fl_width('W')) 
		return false;
	return true;
};

void FontSelectDialog::init_display_string(void)
{
	// create a string for sampling the fonts
	strcpy(str_label, "coLinux\n");
	int i=8;
	uchar c;
	for (c = ' '+1; c < 127; c++) 
	{
		if (!(c&0x1f)) str_label[i++]='\n'; 
		if (c=='@') str_label[i++]=c;
		str_label[i++]=c;
	}
	str_label[i++] = '\n';
	for (c = 0xA1; c; c++) 
	{
		if (!(c&0x1f)) 
			str_label[i++]='\n'; 
		str_label[i++]=c;
	}
	str_label[i] = 0;
}
