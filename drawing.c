#include <stdio.h>
#include "drawing.h"
#include "fonts.h"

float palette[][3] = {
	{1.0, 1.0, 1.0},    // COLOR_SELECTED_TEXT
	{0.0, 1.0, 1.0},    // COLOR_TEXT
	{0.5, 0.5, 0.5},    // COLOR_TEXT_MUTED
	{1.0, 1.0, 1.0},    // COLOR_SELECTED_BOX
	{0.0, 0.0, 0.0},    // COLOR_BACKGROUND
	{1.0, 1.0, 0.0},    // COLOR_FREQ
	{1.0, 0.0, 1.0},    // COLOR_LABEL
	// spectrum
	{0.0, 0.0, 0.0},    // SPECTRUM_BACKGROUND
	{0.1, 0.1, 0.1},    // SPECTRUM_GRID
	{1.0, 1.0, 0.0},    // SPECTRUM_PLOT
	{0.2, 0.2, 0.2},    // SPECTRUM_NEEDLE
	{0.5, 0.5, 0.5},    // COLOR_CONTROL_BOX
	{0.2, 0.2, 0.2},    // SPECTRUM_BANDWIDTH
	{1.0, 0.0, 0.0},    // SPECTRUM_PITCH
	{0.1, 0.1, 0.2}     // SELECTED_LINE
};


int measure_text(cairo_t *gfx, char *text, int font_entry){
	cairo_text_extents_t ext;
	struct font_style *s = font_table + font_entry;
	
	cairo_select_font_face(gfx, s->name, s->type, s->weight);
	cairo_set_font_size(gfx, s->height);
	cairo_move_to(gfx, 0, 0);
	cairo_text_extents(gfx, text, &ext);
	return (int) ext.x_advance;
}

int measure_text_y(cairo_t *gfx, int font_entry){
	cairo_font_extents_t ext;
	struct font_style *s = font_table + font_entry;
	
	cairo_select_font_face(gfx, s->name, s->type, s->weight);
	cairo_set_font_size(gfx, s->height);
	cairo_font_extents(gfx, &ext);
	return (int) ext.height;
}

void draw_text(cairo_t *gfx, int x, int y, char *text, int font_entry){
	struct font_style *s  = font_table + font_entry;
	cairo_set_source_rgb( gfx, s->r, s->g, s->b);
	cairo_select_font_face(gfx, s->name, s->type, s->weight);
	cairo_set_font_size(gfx, s->height);
	cairo_move_to(gfx, x, y + s->height);
	cairo_show_text(gfx, text);
}

void fill_rect(cairo_t *gfx, int x, int y, int w, int h, int color){
	cairo_set_source_rgb( gfx, palette[color][0], palette[color][1], palette[color][2]);
	cairo_rectangle(gfx, x, y, w, h);
	cairo_fill(gfx);
}

void rect(cairo_t *gfx, int x, int y, int w, int h, int color, int thickness){

	cairo_set_source_rgb( gfx, 
		palette[color][0], 
		palette[color][1], 
		palette[color][2]);

	cairo_set_line_width(gfx, thickness);
	cairo_rectangle(gfx, x, y, w, h);
	cairo_stroke(gfx);
}

void freq_with_separators(char *return_string, int freq){
	int f_mhz = freq / 1000000;
	int f_khz = (freq % 1000000) / 1000;
	int f_hz = (freq % 1000) / 10;

	sprintf(return_string,"%d.%03d.%02d",f_mhz, f_khz, f_hz);
}

