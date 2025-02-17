#pragma once

#include <stdbool.h>
#include <cairo.h>

#define MAX_FIELD_LENGTH 128

enum {
	FIELD_NUMBER,
	FIELD_BUTTON,
	FIELD_TOGGLE,
	FIELD_SELECTION,
	FIELD_TEXT,
	FIELD_STATIC,
	FIELD_CONSOLE
};

struct field {
	char const * const cmd;
	int			(*fn)(struct field *f, cairo_t *gfx, int event, int param_a, int param_b, int param_c);
	int			x, y, width, height;
	char		label[30];
 	int 		label_width;
	char		value[MAX_FIELD_LENGTH];
	char		value_type; // NUMBER, SELECTION, TEXT, TOGGLE, BUTTON
	int 		font_index; // refers to font_style table
	char const * const selection;
	long int	min, max;
	int			step;
	char		is_dirty;
	char		update_remote;
};

extern struct field main_controls[];

int do_spectrum(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_waterfall(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_tuning(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_text(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_status(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_console(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_pitch(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_kbd(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_macro(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
int do_record(struct field *f, cairo_t *gfx, int event, int a, int b, int c);
void do_cmd(const char *cmd);

extern struct field *active_layout;

struct field *get_field(const char * const cmd);
void update_field(struct field *f);
struct field *get_field_by_label(const char * const label);
int get_field_value(const char * const cmd, char *value);
int get_field_value_by_label(const char * const label, char *value);
int remote_update_field(int i, char *text);
int set_field(const char * const id, char *value);

bool parseEqual(const char const *text, char *left, char *right); 
