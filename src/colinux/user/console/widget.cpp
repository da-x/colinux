/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "widget.h"
	
static int default_red[] = {0x00,0xaa,0x00,0xaa,0x00,0xaa,0x00,0xaa,
			    0x55,0xff,0x55,0xff,0x55,0xff,0x55,0xff};

static int default_grn[] = {0x00,0x00,0xaa,0xaa,0x00,0x00,0xaa,0xaa,
			    0x55,0x55,0xff,0xff,0x55,0x55,0xff,0xff};

static int default_blu[] = {0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
			    0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff};

#define FL_RGB_COLOR(i) fl_color(default_blu[i], \
				 default_grn[i], \
				 default_red[i])

console_widget_t::console_widget_t(int x, int y, int w, int h, const char* label)
	: Fl_Widget(x, y, w, h, label)
{
	font_size = 15;
	letter_x = font_size;
	letter_y = font_size;
	cursor_blink_interval = 0.1;
	cursor_blink_state = 1;

	Fl::add_timeout(cursor_blink_interval, (Fl_Timeout_Handler)(console_widget_t::static_blink_handler), this);
}

void console_widget_t::static_blink_handler(console_widget_t *widget)
{
	widget->blink_handler();
}

void console_widget_t::blink_handler()
{
	if (console) {
		damage_console(console->cursor.x, console->cursor.y, 1, 1);

		/* 

		For the cursor to blink we would do: cursor_blink_state = !cursor_blink_state

		However, we need to fix a problem with console_idle() which causes timers not
		to execute unless there are input events.
		
		*/
	}

	Fl::add_timeout(cursor_blink_interval, (Fl_Timeout_Handler)(console_widget_t::static_blink_handler), this);
}

void console_widget_t::set_font_size(int size)
{
	font_size = size;
}

void console_widget_t::damage_console(int x_, int y_, int w_, int h_)
{
	int cx, cy;

	cx = x();
	cy = y();

	cx -= ((letter_x * console->x) - w())/2;
	cy -= ((letter_y * console->y) - h())/2;

	damage(1, 
	       cx + x_*letter_x, 
	       cy + y_*letter_y, 
	       w_*letter_x, 
	       h_*letter_y);
}

void console_widget_t::draw()
{
	if (console == NULL) {
		int x_, y_, w_, h_;
		fl_clip_box(x(),y(),w(),h(), x_, y_, w_, h_);
		fl_color(FL_BLACK);
		fl_rectf(x_, y_, w_, h_);
		return;
	}

	fl_font(FL_SCREEN, font_size);

	letter_x = (int)fl_width('A');
	letter_y = (int)fl_height();

	int x_, y_, w_, h_, cx, cy;	
	fl_clip_box(x(),y(),w(),h(), x_, y_, w_, h_);

	cx = x();
	cy = y();

	cx -= ((letter_x * console->x) - w())/2;
	cy -= ((letter_y * console->y) - h())/2;
	
	x_ -= cx;
	y_ -= cy;

	int x1 = x_ / letter_x;
	int x2 = (x_ + w_) / letter_x + 1;
	int y1 = y_ / letter_y;
	int y2 = (y_ + h_) / letter_y + 1;

	int yi = 0;

	if (x_ < 0) {
		fl_color(FL_BLACK);
		fl_rectf(x(), y(), -x_, h_);

		x1 = 0;
		cx += x1 * letter_x;
	}

	if (y_ < 0) {
		fl_color(FL_BLACK);
		fl_rectf(cx, y(), console->x*letter_x, -y_);
	}

	if (x_ + w_ > console->x*letter_x) {
		fl_color(FL_BLACK);
		fl_rectf(cx+console->x*letter_x, y(), x_ + w_ - console->x*letter_x, h_);

		x2 = console->x;
	}

	if (y_ + h_ > console->y*letter_y) {
		fl_color(FL_BLACK);
		fl_rectf(cx, cy + console->y*letter_y, 
			 console->x*letter_x, y_ + h_ - console->y*letter_y);
	}

	fl_push_clip(x(),y(),w(),h());

	for (yi=y1; yi < y2; yi++) {
		if (yi < 0)
			continue;

		if (yi > console->y)
			break;

		co_console_cell_t *row_start = &console->screen[yi*console->x];
		co_console_cell_t *cell, *start, *end;
		char text_buff[0x100];

		start = row_start + x1;
		end = row_start + x2;
		cell = start;
		
		while (cell < end) {
			while (cell < end  &&  start->attr == cell->attr) {
				text_buff[cell - start] = cell->ch;
				cell++;
			}

			FL_RGB_COLOR((start->attr >> 4) & 0xf);
			fl_rectf(cx + letter_x * (start - row_start), 
				 cy + letter_y * (yi),
				 (cell - start) * letter_x,
				 letter_y);
			
			FL_RGB_COLOR((start->attr) & 0xf);
			fl_draw(text_buff, cell - start, 
				cx + letter_x * (start - row_start), 
				cy + letter_y * (yi + 1) - fl_descent());

			start = cell;
		}

		if (!cursor_blink_state)
			continue;

		if (console->cursor.y == yi) {
			if (console->cursor.x >= x1 && 	console->cursor.x <= x2) {
				fl_color(0xff, 0xff, 0xff);
				fl_rectf(cx + letter_x * console->cursor.x, 
					 cy + letter_y * console->cursor.y + 
					 (letter_y * (CO_CURSOR_POS_SIZE - console->cursor.end)) / CO_CURSOR_POS_SIZE,
					 letter_x, (letter_y * (console->cursor.end - 
						     console->cursor.start)) / CO_CURSOR_POS_SIZE);
			}
		}
	}
	
	fl_pop_clip();
}

void console_widget_t::set_console(co_console_t *_console)
{
	console = _console;
}

co_console_t *console_widget_t::get_console()
{
	co_console_t *_console = console;
	console = 0;

	return _console;
}

co_rc_t console_widget_t::handle_console_event(co_console_message_t *message)
{
	co_rc_t rc;
	co_cursor_pos_t saved_cursor_pos = {0, };

	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_CURSOR: {
		saved_cursor_pos = console->cursor;
		break;
	}
	default:
		break;
	}

	rc = co_console_op(console, message);
	if (!CO_OK(rc))
		return rc;

	switch (message->type) 
	{
	case CO_OPERATION_CONSOLE_SCROLL: {
		unsigned long t = message->scroll.t;     /* Start of scroll region (row) */
		unsigned long b = message->scroll.b;     /* End of scroll region (row) */

		damage_console(0, t, console->x, b - t + 1);
		break;
	}
	case CO_OPERATION_CONSOLE_PUTCS: {
		int x = message->putcs.x, y = message->putcs.y;
		int count = message->putcs.count, sx = x, scount = 0;

		while (x < console->x  &&  count > 0) {
			x++;
			count--;
			scount++;
		}

		damage_console(sx, y, scount, 1);
		break;
	}
	case CO_OPERATION_CONSOLE_PUTC: {
		int x = message->putc.x, y = message->putc.y;
		
		damage_console(x, y, 1, 1);
		break;
	}
	case CO_OPERATION_CONSOLE_CURSOR: {
		damage_console(saved_cursor_pos.x, saved_cursor_pos.y, 1, 1);
		damage_console(console->cursor.x, console->cursor.y, 1, 1);
		break;
	}
	}

	return CO_RC(OK);
}

