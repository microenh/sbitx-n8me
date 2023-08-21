#pragma ONCE

#include <cairo.h>

int measure_text(cairo_t *gfx, char *text, int font_entry);
void draw_text(cairo_t *gfx, int x, int y, char *text, int font_entry);
void fill_rect(cairo_t *gfx, int x, int y, int w, int h, int color);
void rect(cairo_t *gfx, int x, int y, int w, int h, int color, int thickness);

enum {
	COLOR_SELECTED_TEXT,
	COLOR_TEXT,
	COLOR_TEXT_MUTED,
	COLOR_SELECTED_BOX,
	COLOR_BACKGROUND,
	COLOR_FREQ,
	COLOR_LABEL,
	SPECTRUM_BACKGROUND,
	SPECTRUM_GRID,
	SPECTRUM_PLOT,
	SPECTRUM_NEEDLE,
	COLOR_CONTROL_BOX,
	SPECTRUM_BANDWIDTH,
	SPECTRUM_PITCH,
	SELECTED_LINE
};

extern float palette[][3];