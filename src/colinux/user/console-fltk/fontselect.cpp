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

#include "main.h"

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
	WriteRegistry(REGISTRY_FONT, fsd_inst->textobj->get_font());
	WriteRegistry(REGISTRY_FONT_SIZE, fsd_inst->textobj->get_size());
}

static void on_font_close(Fl_Widget *widget, void* v)
{
	((FontSelectDialog*)v)->hide();
}

FontSelectDialog::FontSelectDialog(void*ptr, int def_font, int def_size): Fl_Window(530, 370, "Font select")
{
	ptr_console = ptr;
	sizeobj = NULL;
	textobj = NULL;
	fontobj = NULL;
	pickedsize = def_size;
	my_sizes = NULL;
	numsizes = NULL;
	lookup = NULL;
	num_fonts = 0;

	strcpy(str_label, DISPLAY_STR);
	//init_display_string();

	// a canvas to display the fonts
	textobj = new FontDisplay(FL_FRAME_BOX, 10, 240, 390, 120, str_label);
	textobj->align(FL_ALIGN_TOP|FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);

	// font types
	fontobj = new Fl_Hold_Browser(10, 10, 390, 220);
	fontobj->box(FL_FRAME_BOX);
	fontobj->color(53,3);
	fontobj->callback((Fl_Callback*)font_cb, this);
	resizable(fontobj);

	// font sizes
	sizeobj = new Fl_Hold_Browser(410, 10, 110, 220);
	sizeobj->box(FL_FRAME_BOX);
	sizeobj->color(53,3);
	sizeobj->callback((Fl_Callback*)size_cb, this);

	// close and select buttons
    Fl_Button * btn_select	= new Fl_Button(410, 290, 110,30, "Select" );
    btn_select->box(FL_FRAME_BOX);
	btn_select->callback((Fl_Callback*)on_font_select, this);
    Fl_Button * btn_close	= new Fl_Button(410, 330, 110,30, "Close" );
    btn_close->box(FL_FRAME_BOX);
	btn_close->callback((Fl_Callback*)on_font_close, this);

	// done with display
	end();
	populate_fonts(true, def_font);

	click_font();
	click_size();
}

void FontSelectDialog::populate_fonts(bool fixed_pitch, int def_font)
{
	if(!fontobj)
		return;
	int index = 0, i;
	bool display_font = false;

	Fl::scheme(NULL);
	Fl::get_system_colors();
	fontobj->clear();

	num_fonts = Fl::set_fonts(NULL);
	my_sizes = new int*[num_fonts];
	numsizes = new int[num_fonts];
	lookup = new int[num_fonts];
	for(i=0; i < num_fonts; i++)
	{
		int modifier;
		const char *name = Fl::get_font_name((Fl_Font)i, &modifier);
		// we only want fixed pitch fonts, after all, this is a terminal
		if (fixed_pitch)
			display_font = is_fixed_pitch((Fl_Font)i);
		else
			display_font = true;
		if(!display_font)
		{
			numsizes[i] = 0;
			lookup[i] = -1;
			continue;
		}
		int *sizes_array;
		int cur_font_num_sizes = Fl::get_font_sizes((Fl_Font)i, sizes_array);
		if(!cur_font_num_sizes)
			// we should never be here
			continue;

		char buffer[256];
		if (modifier)
		{
			char *p = buffer;
			if (modifier & FL_BOLD) {*p++ = '@'; *p++ = 'b';}
			if (modifier & FL_ITALIC) {*p++ = '@'; *p++ = 'i';}
			strcpy(p, name);
			name = buffer;
		}

		numsizes[i] = cur_font_num_sizes;
		my_sizes[i] = new int[cur_font_num_sizes];
		for(int j=0; j<cur_font_num_sizes; j++)
			my_sizes[i][j] = sizes_array[j];
		lookup[i] = index++;
		fontobj->add(name);
		if(def_font==i)
			fontobj->value(index);
	}
}

// FontSelectDialog functions
void FontSelectDialog::click_size(void)
{
	if((!fontobj) || (!sizeobj) || (!textobj))
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
	// ensure objects are available
	if((!fontobj) || (!sizeobj) || (!textobj) || (!num_fonts))
		return;

	// get the font associated with the user selection
	int fn = fontobj->value();
	if (!fn)
		return;
	fn--;
	int i;
	for(i=0; i<num_fonts; i++)
	{
		if (lookup[i]==fn)
		{
			fn = i;
			break;
		}
	}
	if((i==num_fonts) || (fn==-1))
		return;
	textobj->set_font(fn);
	sizeobj->clear();

	int cur_font_num_sizes = numsizes[fn];
	int *s = my_sizes[fn];
	if (cur_font_num_sizes == -1)
	{
		// odd
	}
	else if (!cur_font_num_sizes)
	{
		// no my_sizes
	}
	else if (s[0] == 0)
	{
		// many my_sizes;
		int j = 1;
		for (int i = 1; i<64 || i<s[cur_font_num_sizes-1]; i++)
		{
			char buf[20];
			if (j < cur_font_num_sizes && i==s[j])
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
		for (int i = 0; i < cur_font_num_sizes; i++)
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

bool is_fixed_pitch(Fl_Font test_font)
{
	int *sizes_array;
	int cur_font_num_sizes = Fl::get_font_sizes((Fl_Font)test_font, sizes_array);
	if(!cur_font_num_sizes)
		return false;
	fl_font(test_font, sizes_array[cur_font_num_sizes-1]);
	if ((int)fl_width('i') != (int)fl_width('W'))
		return false;
	if ((int)fl_width('.') != (int)fl_width('>'))
			return false;
	return true;
};

