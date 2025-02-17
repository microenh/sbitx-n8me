/*
The initial sync between the gui values, the core radio values, settings, et al are manually set.
*/


#include <arpa/inet.h>
#include <cairo.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <libgen.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <math.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "command_str.h"
#include "draw_dial.h"
#include "drawing.h"
#include "fields.h"
#include "fonts.h"
#include "hamlib.h"
#include "i2cbb.h"
#include "ini.h"
#include "logbook.h"
#include "queue.h"
#include "panafall.h"
#include "remote.h"
#include "sdr_ui.h"
#include "sdr.h"
#include "sound.h"
#include "si5351.h"


/* command  buffer for commands received from the remote */
struct Queue q_remote_commands;
struct Queue q_tx_text;

/* Front Panel controls */

#define ENC1_A  (13)
#define ENC1_B  (12)
#define ENC1_SW (14)

#define ENC2_A  (0)
#define ENC2_B  (2)
#define ENC2_SW (3)

#define SW5  (22)
#define PTT  (7)
#define DASH (21)

#define ENC_FAST 1
#define ENC_SLOW 5


// mouse/touch screen state
static int mouse_down = 0;
static int last_mouse_x = -1;
static int last_mouse_y = -1;

static char *argv_0;

//encoder state
struct encoder {
	int pin_a, pin_b;
	int speed;
	int prev_state;
	int history;
};

static void tuning_isr(void);


static int screen_width=800, screen_height=480;
static guint key_modifier = 0;
struct encoder enc_a, enc_b;



// The console is a series of lines
#define MAX_CONSOLE_BUFFER 10000
#define MAX_LINE_LENGTH      128
#define MAX_CONSOLE_LINES    500
static int console_cols = 50;

// we use just one text list in our user interface

struct console_line {
	char text[MAX_LINE_LENGTH];
	int style;
};
static int console_style = FONT_LOG;
static struct console_line console_stream[MAX_CONSOLE_LINES];
static int console_current_line = 0;
static int	console_selected_line = -1;

// event ids, some of them are mapped from gtk itself
enum {
    FIELD_DRAW,
    FIELD_UPDATE,
    FIELD_EDIT
};

#define MIN_KEY_UP			0xFF52
#define MIN_KEY_DOWN		0xFF54
#define MIN_KEY_LEFT		0xFF51
#define MIN_KEY_RIGHT		0xFF53
#define MIN_KEY_ENTER		0xFF0D
#define MIN_KEY_ESC			0xFF1B
#define MIN_KEY_BACKSPACE	0xFF08
#define MIN_KEY_TAB			0xFF09
#define MIN_KEY_CONTROL		0xFFE3
#define MIN_KEY_F1			0xFFBE
#define MIN_KEY_F2			0xFFBF
#define MIN_KEY_F3			0xFFC0
#define MIN_KEY_F4			0xFFC1
#define MIN_KEY_F5			0xFFC2
#define MIN_KEY_F6			0xFFC3
#define MIN_KEY_F7			0xFFC4
#define MIN_KEY_F8			0xFFC5
#define MIN_KEY_F9			0xFFC6
#define MIN_KEY_F9			0xFFC6
#define MIN_KEY_F10			0xFFC7
#define MIN_KEY_F11			0xFFC8
#define MIN_KEY_F12			0xFFC9

#define COMMAND_ESCAPE	'\\'

static void set_ui(int id);
static void set_bandwidth(int hz);

/* 	the field in focus will be exited when you hit an escape
    the field in focus will be changeable until it loses focus
    hover will always be on the field in focus.
    if the focus is -1,then hover works
*/

/*
	Warning: The field selection is used for TOGGLE and SELECTION fields
	each selection by the '/' should be unique. otherwise, the simple logic will
	get confused 
*/


//the main app window
static GtkWidget *window;
static GtkWidget *display_area = NULL;

// these are callbacks called by the operating system
static gboolean on_draw_event(GtkWidget* widget, cairo_t *cr, gpointer user_data); 
static gboolean on_key_release(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_mouse_press(GtkWidget *widget, GdkEventButton *event, gpointer data); 
static gboolean on_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer data); 
static gboolean on_mouse_release(GtkWidget *widget, GdkEventButton *event, gpointer data); 
static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data); 
static gboolean on_window_state(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_resize(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
static gboolean ui_tick(gpointer gook);

static void update_log_ed();



/****************************************************************************
	Using the above hooks and primitives, we build user interface controls,
	All of them are defined by the struct field
****************************************************************************/


#define STACK_DEPTH 4

struct band {
	char name[10];
	int	start;
	int	stop;
	// int	power;
	// int	max;
	int index;
	int	freq[STACK_DEPTH];
	int mode[STACK_DEPTH];
};

struct cmd {
	char *cmd;
	int (*fn)(char *args[]);
};


static unsigned long focus_since = 0;
static struct field *f_focus = NULL;
static struct field *f_hover = NULL;
// variables to power up and down the tx

int in_tx = TX_OFF;
static int key_down = 0;
static int tx_start_time = 0;

static int *tx_mod_buff = NULL;
static int tx_mod_index = 0;
static int tx_mod_max = 0;

char*mode_name[MAX_MODES] = {
	"USB", "LSB", "CW", "CWR", "NBFM", "AM", "FT8", "PSK31", "RTTY", 
	"DIGITAL", "2TONE" 
};

static int serial_fd = -1;
static int xit = 512; 
static long int tuning_step = 1000;
static int tx_mode = MODE_USB;

enum {
	BAND80M,
	BAND40M,
	BAND30M,
	BAND20M,
	BAND17M,
	BAND15M,
	BAND12M,
	BAND10M
};

struct band band_stack[] = {
	{"80m",  3500000,  4000000, 0, { 3500000,  3574000,  3600000,  3700000}, {MODE_CW, MODE_USB, MODE_CW,  MODE_LSB}},
	{"40m",  7000000,  7300000, 0, { 7000000,  7040000,  7074000,  7150000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_LSB}},
	{"30m", 10100000, 10150000, 0, {10100000, 10100000, 10136000, 10150000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
	{"20m", 14000000, 14400000, 0, {14010000, 14040000, 14074000, 14200000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
	{"17m", 18068000, 18168000, 0, {18068000, 18100000, 18110000, 18160000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
	{"15m", 21000000, 21500000, 0, {21010000, 21040000, 21074000, 21250000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
	{"12m", 24890000, 24990000, 0, {24890000, 24910000, 24950000, 24990000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
	{"10m", 28000000, 29700000, 0, {28000000, 28040000, 28074000, 28250000}, {MODE_CW, MODE_CW,  MODE_USB, MODE_USB}},
};


#define VFO_A 0 
#define VFO_B 1 
// int vfo_a_freq = 7000000;
// int vfo_b_freq = 14000000;
char vfo_a_mode[10];
char vfo_b_mode[10];

// useful data for macros, logging, etc
char contact_callsign[12];
char contact_grid[10];
char sent_rst[10];
char received_rst[10];
char received_exchange[10];

int	tx_id = 0;

// recording duration in seconds
time_t record_start = 0;
int	data_delay = 700;

#define MAX_RIT 25000

// extern int spectrum_plot[];
extern int fwdpower, vswr;


static void cmd_exec(char *cmd);



char settings_updated = 0;
enum {
    LAYOUT_KBD,
    LAYOUT_MACROS
};

int current_layout = LAYOUT_KBD;




// #define MAX_CONSOLE_LINES 1000
// char *console_lines[MAX_CONSOLE_LINES];
int last_log = 0;



// log is a special field that essentially is a like text
// on a terminal

static void console_init(){
	for (int i =0;  i < MAX_CONSOLE_LINES; i++){
		console_stream[i].text[0] = 0;
		console_stream[i].style = console_style;
	}
	struct field *f = get_field("#console");
	f->is_dirty = 1;
}


static int console_init_next_line(){
	console_current_line++;
	if (console_current_line == MAX_CONSOLE_LINES)
		console_current_line = console_style;
	console_stream[console_current_line].text[0] = 0;	
	console_stream[console_current_line].style = console_style;
	return console_current_line;
}

static void write_to_remote_app(int style, char *text){
	if (style == FONT_FLDIGI_RX)
		return; //this is temporary
	remote_write("{");
	remote_write(text);
	remote_write("}");
}

void write_console(int style, char *text){
	char directory[200];	// dangerous, find the MAX_PATH and replace 200 with it
	char *path = getenv("HOME");
	strcpy(directory, path);
	strcat(directory, "/sbitx/data/display_log.txt");
	FILE *pf = fopen(directory, "a");

	// move to a new line if the style has changed
	if (style != console_style){
		console_style = style;
		if (strlen(console_stream[console_current_line].text)> 0)
			console_init_next_line();	
		console_stream[console_current_line].style = style;
		switch(style){
			case FONT_FT8_RX:
			case FONT_FLDIGI_RX:
			case FONT_CW_RX:
				fputs("#RX ################################\n", pf);
				break;
			case FONT_FT8_TX:
			case FONT_FLDIGI_TX:
			case FONT_CW_TX:
				fputs("#TX ################################\n", pf);
				break;
			default:
				fputs("#INFO ##############################\n", pf);
				break;
		}
	}

	if (strlen(text) == 0)
		return;

	// write to the scroll
	fwrite(text, strlen(text), 1, pf);
	fclose(pf);
	
	write_to_remote_app(style, text);

	while(*text){
		char c = *text;
		if (c == '\n')
			console_init_next_line();
		else if (c < 128 && c >= ' '){
			char *p = console_stream[console_current_line].text;
			int len = strlen(p);
			if(len >= console_cols - 1){
				// start a fresh line
				console_init_next_line();
				p = console_stream[console_current_line].text;
				len = 0;
			}
		
			// printf("Adding %c to %d\n", (int)c, console_current_line);	
			p[len++] = c & 0x7f;
			p[len] = 0;
		}
		text++;	
	}

	struct field *f = get_field("#console");
	if (f)
		f->is_dirty = 1;
}

static void draw_log(cairo_t *gfx, struct field *f){
	char this_line[1000];
	int line_height = font_table[f->font_index].height; 	
	int n_lines = (f->height / line_height) - 1;

	// estimate!
	int char_width = 1+measure_text(gfx, "01234567890123456789", f->font_index)/20;
	console_cols = f->width / char_width;
	int y = f->y + 2; 
	int j = 0;

	int start_line = console_current_line - n_lines;
	if (start_line < 0)
		start_line += MAX_CONSOLE_LINES;

 	for (int i = 0; i <= n_lines; i++){
		struct console_line *l = console_stream + start_line;
		if (start_line == console_selected_line)
			fill_rect(gfx, f->x, y+1, f->width, font_table[l->style].height+1, SELECTED_LINE);
		draw_text(gfx, f->x, y, l->text, l->style);
		start_line++;
		y += line_height;
		if(start_line >= MAX_CONSOLE_LINES)
			start_line = 0;
	}
}

static void draw_field(GtkWidget *widget, cairo_t *gfx, struct field *f){
	struct font_style *s = font_table + 0;

	// push this to the web as well
	
	f->is_dirty = 0;
	if (f->x <= -1000)
		return;

	// if there is a handling function, use that else
	// skip down to the default behaviour of the controls
	if (f->fn){
		if(f->fn(f, gfx, FIELD_DRAW, -1, -1, 0)){
			f->is_dirty = 0;
			return;
		}
	}

	fill_rect(gfx, f->x, f->y, f->width, f->height, COLOR_BACKGROUND);
	if (f_focus == f)
		rect(gfx, f->x+1, f->y+1, f->width-2, f->height-2, COLOR_SELECTED_BOX, 2);
	else if (f_hover == f)
		rect(gfx, f->x+1, f->y+1, f->width-2, f->height-2, COLOR_SELECTED_BOX, 1);
	else if (f->value_type != FIELD_STATIC)
		rect(gfx, f->x, f->y, f->width, f->height, COLOR_CONTROL_BOX, 1);

	int width, offset_x, text_length, line_start, y, label_height, 
		value_height, value_font;	
	char this_line[MAX_FIELD_LENGTH];
	int text_line_width = 0;

	int label_y;
	int use_reduced_font = 0;

	switch(f->value_type){
		case FIELD_TEXT:
			text_length = strlen(f->value);
			line_start = 0;
			y = f->y + 2;
			text_line_width = 0;
			while(text_length > 0){
				if (text_length > console_cols){
					strncpy(this_line, f->value + line_start, console_cols);
					this_line[console_cols] = 0;
				}
				else
					strcpy(this_line, f->value + line_start);		
				draw_text(gfx, f->x + 2, y, this_line, f->font_index);
				text_line_width= measure_text(gfx, this_line, f->font_index);
				y += 14;
				line_start += console_cols;
				text_length -= console_cols;
			}
			// draw the text cursor, if there is no text, the text baseline is zero
			if (strlen(f->value))
				y -= 14;
			fill_rect(gfx, f->x + text_line_width + 5, y+3, 9, 10, f->font_index);
		break;
		case FIELD_SELECTION:
		case FIELD_NUMBER:
		case FIELD_TOGGLE:
		case FIELD_BUTTON:
        #if 0
			label_height = font_table[FONT_FIELD_LABEL].height;
			width = measure_text(gfx, f->label, FONT_FIELD_LABEL);
			offset_x = f->x + f->width/2 - width/2;
			// is it a two line display or a single line?
			if (f->value_type == FIELD_BUTTON && !f->value[0]){
				label_y = f->y + (f->height - label_height)/2;
				draw_text(gfx, offset_x,label_y, f->label, FONT_FIELD_LABEL);
			} 
			else {
				if(width >= f->width+2)
					value_font = FONT_SMALL_FIELD_VALUE;
				else
					value_font = FONT_FIELD_VALUE;
				value_height = font_table[value_font].height;
				label_y = f->y + ((f->height  - label_height  - value_height)/2);
				draw_text(gfx, offset_x, label_y, f->label, FONT_FIELD_LABEL);
				width = measure_text(gfx, f->value, value_font);
				label_y += font_table[FONT_FIELD_LABEL].height;
				draw_text(gfx, f->x + f->width/2 - width/2, label_y, f->value, value_font);
			}
        #endif
			label_height = measure_text_y(gfx, f->font_index); // font_table[f->font_index].height;
			width = measure_text(gfx, f->label, f->font_index);
			offset_x = f->x + f->width/2 - width/2;
			// is it a two line display or a single line?
			if (f->value_type == FIELD_BUTTON && !f->value[0]){
				label_y = f->y + (f->height - label_height)/2;
				draw_text(gfx, offset_x,label_y, f->label, f->font_index);
			} 
			else {
				if(width >= f->width+2)
					value_font = FONT_SMALL_FIELD_VALUE;
				else
					value_font = f->font_index;
				value_height = measure_text_y(gfx, f->font_index); // font_table[value_font].height;
				label_y = f->y + ((f->height  - label_height  - value_height)/2);
				draw_text(gfx, offset_x, label_y, f->label, f->font_index);
				width = measure_text(gfx, f->value, value_font);
				label_y += font_table[f->font_index].height;
				draw_text(gfx, f->x + f->width/2 - width/2, label_y, f->value, value_font);
			}
      break;
		case FIELD_STATIC:
			draw_text(gfx, f->x, f->y, f->label, FONT_FIELD_LABEL);
			break;
		case FIELD_CONSOLE:
			// draw_log(gfx, f);
			break;
	}
}

// scales the ui as per current screen width from
// the nominal 800x480 size of the original layout
static void scale_ui(){
	for (int i = 0; active_layout[i].cmd[0] > 0; i++) {
		struct field *f = active_layout + i;
		f->x = (f->x * screen_width)/800;
		f->y = (f->y * screen_height)/480;
		f->width = (f->width * screen_width)/800;
		f->height = (f->height * screen_height)/480;
	}
}

static int mode_id(char *mode_str){
	if (!strcmp(mode_str, "CW"))
		return MODE_CW;
	else if (!strcmp(mode_str, "CWR"))
		return MODE_CWR;
	else if (!strcmp(mode_str, "USB"))
		return MODE_USB;
	else if (!strcmp(mode_str,  "LSB"))
		return MODE_LSB;
	else if (!strcmp(mode_str,  "FT8"))
		return MODE_FT8;
	else if (!strcmp(mode_str,  "PSK31"))
		return MODE_PSK31;
	else if (!strcmp(mode_str,  "RTTY"))
		return MODE_RTTY;
	else if (!strcmp(mode_str, "NBFM"))
		return MODE_NBFM;
	else if (!strcmp(mode_str, "AM"))
		return MODE_AM;
	else if (!strcmp(mode_str, "2TONE"))
		return MODE_2TONE;
	else if (!strcmp(mode_str, "DIGITAL"))
		return MODE_DIGITAL;
	return -1;
}

static void save_user_settings(int forced){
	static int last_save_at = 0;
	char file_path[200];	// dangerous, find the MAX_PATH and replace 200 with it

	// attempt to save settings only if it has been 30 seconds since the 
	// last time the settings were saved
	int now = millis();
	if ((now < last_save_at + 30000 ||  !settings_updated) && forced == 0)
		return;

	char *path = getenv("HOME");
	strcpy(file_path, path);
	strcat(file_path, "/sbitx/data/user_settings.ini");

	// copy the current freq settings to the currently selected vfo
	struct field *f_freq = get_field("r1:freq");
	struct field *f_vfo  = get_field("#vfo");

	FILE *f = fopen(file_path, "w");
	if (!f){
		printf("Unable to save %s : %s\n", file_path, strerror(errno));
		return;
	}

	// save the field values
	int i;
	for (i= 0; active_layout[i].cmd[0] > 0; i++){
		fprintf(f, "%s=%s\n", active_layout[i].cmd, active_layout[i].value);
	}

	// now save the band stack
	for (int i = 0; i < sizeof(band_stack)/sizeof(struct band); i++){
		fprintf(f, "\n[%s]\n", band_stack[i].name);
		// fprintf(f, "power=%d\n", band_stack[i].power);
		for (int j = 0; j < STACK_DEPTH; j++)
			fprintf(f, "freq%d=%d\nmode%d=%d\n", j, band_stack[i].freq[j], j, band_stack[i].mode[j]);
	}

	fclose(f);
	settings_updated = 0;
}


static int user_settings_handler(void* user, const char* section, 
	const char* name, const char* value)
{
    char cmd[1000];
    char new_value[200];

    strcpy(new_value, value);
    if (!strcmp(section, "r1")){
		sprintf(cmd, "%s:%s", section, name);
		set_field(cmd, new_value);
    }
    else if (!strcmp(section, "tx")){
		strcpy(cmd, name);
		set_field(cmd, new_value);
    }
    // if it is an empty section
    else if (strlen(section) == 0){
		#ifdef N8ME
		if (strcmp(name, "#kbd_")) { 
			strcpy(cmd, name);
		#else
		sprintf(cmd, "%s", name);
		#endif
		set_field(cmd, new_value); 
		#ifdef N8ME
		}
		#endif
    }

	// band stacks
	int band = -1;
	if (!strcmp(section, "80m"))
		band = 0;
	else if (!strcmp(section, "40m"))
		band = 1;
	else if (!strcmp(section, "30m"))
		band = 2;
	else if (!strcmp(section, "20m"))
		band = 3;
	else if (!strcmp(section, "17m"))
		band = 4;
	else if (!strcmp(section, "15m"))
		band = 5;
	else if (!strcmp(section, "13m"))	
		band = 6;
	else if (!strcmp(section, "10m"))
		band = 7;	

	if (band >= 0  && !strcmp(name, "freq0"))
		band_stack[band].freq[0] = atoi(value);
	else if (band >= 0  && !strcmp(name, "freq1"))
		band_stack[band].freq[1] = atoi(value);
	else if (band >= 0  && !strcmp(name, "freq2"))
		band_stack[band].freq[2] = atoi(value);
	else if (band >= 0  && !strcmp(name, "freq3"))
		band_stack[band].freq[3] = atoi(value);
	else if (band >= 0 && !strcmp(name, "mode0"))
		band_stack[band].mode[0] = atoi(value);	
	else if (band >= 0 && !strcmp(name, "mode1"))
		band_stack[band].mode[1] = atoi(value);	
	else if (band >= 0 && !strcmp(name, "mode2"))
		band_stack[band].mode[2] = atoi(value);	
	else if (band >= 0 && !strcmp(name, "mode3"))
		band_stack[band].mode[3] = atoi(value);	

    return 1;
}
/* rendering of the fields */



static void invalidate_rect(int x, int y, int width, int height){
	if (display_area){
		gtk_widget_queue_draw_area(display_area, x, y, width, height);
	}
}

static void redraw_main_screen(GtkWidget *widget, cairo_t *gfx){
	double dx1, dy1, dx2, dy2;
	int x1, y1, x2, y2;

	/*
		int width, height;
		gtk_window_get_size(GTK_WINDOW(window), &width, &height);
		printf("Screen size is %d, %d\n", width, height);
	*/

	cairo_clip_extents(gfx, &dx1, &dy1, &dx2, &dy2);
	x1 = (int)dx1;
	y1 = (int)dy1;
	x2 = (int)dx2;
	y2 = (int)dy2;

	fill_rect(gfx, x1, y1, x2-x1, y2-y1, COLOR_BACKGROUND);
	for (int i = 0; active_layout[i].cmd[0] > 0; i++){
		double cx1, cx2, cy1, cy2;
		struct field *f = active_layout + i;
		cx1 = f->x;
		cx2 = cx1 + f->width;
		cy1 = f->y;
		cy2 = cy1 + f->height;
		if (cairo_in_clip(gfx, cx1, cy1) || cairo_in_clip(gfx, cx2, cy2)){
			draw_field(widget, gfx, active_layout + i);
		}
	}
}

/* gtk specific routines */
static gboolean on_draw_event( GtkWidget* widget, cairo_t *cr, gpointer user_data ) {
	redraw_main_screen(widget, cr);	
	return FALSE;
}

static gboolean on_resize(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
	// printf("size changed to %d x %d\n", event->width, event->height);
	screen_width = event->width;
	screen_height = event->height;
}

static void hover_field(struct field *f){
	struct field *prev_hover = f_hover;
	if (f) {
		// set f_hover to none to remove the outline
		f_hover = NULL;
		update_field(prev_hover);
	}
	f_hover = f;
	update_field(f);
}


// respond to a UI request to change the field value
static void edit_field(struct field *f, int action){
	int v;

	if (f == f_focus)
		focus_since = millis();

	if (f->fn){
		f->is_dirty = 1;
	 	f->update_remote = 1;
		if (f->fn(f, NULL, FIELD_EDIT, action, 0, 0))
			return;
	}
	
	if (f->value_type == FIELD_NUMBER){
		int	v = atoi(f->value);
		if (action == MIN_KEY_UP && v + f->step <= f->max)
			v += f->step;
		else if (action == MIN_KEY_DOWN && v - f->step >= f->min)
			v -= f->step;
		sprintf(f->value, "%d",  v);
	} else if (f->value_type == FIELD_SELECTION){
		char *p, *prev, *next, b[100], *first, *last;
    	// get the first and last selections
    	strcpy(b, f->selection);
    	p = strtok(b, "/");
    	first = p;
    	while(p){
    		last = p;
    		p = strtok(NULL, "/");
    	}
		// search the current text in the selection
		prev = NULL;
		strcpy(b, f->selection);
		p = strtok(b, "/");
		while(p){
			if (!strcmp(p, f->value))
				break;
			else
				prev = p;
			p = strtok(NULL, "/");
		}	
		//set to the first option
		if (p == NULL){
			if (prev)
				strcpy(f->value, prev);
		}
		else if (action == MIN_KEY_DOWN){
			prev = p;
			p = strtok(NULL,"/");
			if (p)
				strcpy(f->value, p);
			else
        		strcpy(f->value, first); // roll over
			// return;
			// strcpy(f->value, prev); 
		}
		else if (action == MIN_KEY_UP){
			if (prev)
				strcpy(f->value, prev);
			else
        strcpy(f->value, last); // roll over
				//return;
		}
	} else if (f->value_type == FIELD_TOGGLE){
		char *p, *prev, *next, b[100];
		// search the current text in the selection
		prev = NULL;
		strcpy(b, f->selection);
		p = strtok(b, "/");
		while(p){
			if (strcmp(p, f->value))
				break;
			p = strtok(NULL, "/");
		}	
		strcpy(f->value, p);
	} else if (f->value_type == FIELD_BUTTON){
		NULL; // ah, do nothing!
	}

	//send a command to the receiver
	char buff[200];
	sprintf(buff, "%s=%s", f->cmd, f->value);
	do_cmd(buff);
	f->is_dirty = 1;
	f->update_remote = 1;
	// update_field(f);
	settings_updated++;
}

static void focus_field(struct field *f){
	struct field *prev_hover = f_hover;
	struct field *prev_focus = f_focus;
	
	f_focus = NULL;
	if (prev_hover)
		update_field(prev_hover);
	if (prev_focus)
		update_field(prev_focus);
	if (f){
		f_focus = f_hover = f;
		focus_since = millis();
	}
	update_field(f_hover);

	// is it a toggle field?
	if (f_focus->value_type == FIELD_TOGGLE)
		edit_field(f_focus, MIN_KEY_DOWN);	

	// is it a selection field?
	if (f_focus->value_type == FIELD_SELECTION) 
    	edit_field(f_focus, MIN_KEY_UP);

	// if the button has been pressed, do the needful
	if (f_focus->value_type == FIELD_TOGGLE || 
		f_focus->value_type == FIELD_BUTTON)
		do_cmd(f->cmd);
}

// setting the frequency is complicated by having to take care of the
// rit/split and power levels associated with each frequency
static void set_operating_freq(int dial_freq, char *response){
	struct field *rit = get_field("#rit");
	struct field *split = get_field("#split");
	struct field *vfo_a = get_field("#vfo_a_freq");
	struct field *vfo_b = get_field("#vfo_b_freq");
	struct field *rit_delta = get_field("#rit_delta");

	char freq_request[30];
 
	if (!strcmp(rit->value, "ON")){
		if (!in_tx)
			sprintf(freq_request, "r1:freq=%d", dial_freq + atoi(rit_delta->value)); 		
		else
			sprintf(freq_request, "r1:freq=%d", dial_freq); 		
	}
	else if (!strcmp(split->value, "ON")){
		if (!in_tx)
			// sprintf(freq_request, "r1:freq=%d", vfo_b_freq);
			sprintf(freq_request, "r1:freq=%s", vfo_b->value);
		else
			// sprintf(freq_request, "r1:freq=%d", dial_freq);
			sprintf(freq_request, "r1:freq=%d", dial_freq);
	} else
		sprintf(freq_request, "r1:freq=%d", dial_freq);

	// get back to setting the frequency
	sdr_request(freq_request, response);
}

void abort_tx(void){
	set_field("#text_in", "");
	modem_abort();
	tx_off();
}

int do_spectrum(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	struct field *f_freq, *f_span, *f_pitch;
	int span, pitch;
	long freq;
	char buff[100];
	int mode = mode_id(get_field(R1_MODE)->value);

	switch(event){
		case FIELD_DRAW:
			draw_spectrum(f, gfx);
			return 1;
			break;
		case GDK_MOTION_NOTIFY:
	    	f_freq = get_field(R1_FREQ);
			freq = atoi(f_freq->value);
			f_span = get_field(_SPAN);
			span = atof(f_span->value) * 1000;
			//a has the x position of the mouse
			freq -= ((a - last_mouse_x) * (span/f->width));
			sprintf(buff, "%ld", freq);
			set_field("r1:freq", buff);
			return 1;
			break;
    	case GDK_BUTTON_PRESS: 
			if (c == GDK_BUTTON_SECONDARY){ // right click QSY
				f_freq = get_field(R1_FREQ);
				freq = atoi(f_freq->value);
				f_span = get_field(_SPAN);
				span = atof(f_span->value) * 1000;
				f_pitch = get_field("rx_pitch");
				pitch = atoi(f_pitch->value);
				if (mode == MODE_CW){
					freq += ((((float)(a - f->x) / (float)f->width) - 0.5) * (float)span) - pitch;
				} else if (mode == MODE_CWR){
					freq += ((((float)(a - f->x) / (float)f->width) - 0.5) * (float)span) + pitch;
				} else { // other modes may need to be optimized - k3ng 2022-09-02
					freq += (((float)(a - f->x) / (float)f->width) - 0.5) * (float)span;
				}
				sprintf(buff, "%ld", freq);
				set_field(R1_FREQ, buff);
				return 1;
    		}
    		break;
	}
	return 0;	
}

int do_waterfall(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	switch(event){
		case FIELD_DRAW:
			draw_waterfall(f, gfx);
			return 1;
		/*
		case GDK_MOUSE_MOVE:{
			struct field *f_freq = get_field("r1:freq");
			long freq = atoi(f_freq->value);
			struct field *f_span = get_field("#span");
			int span = atoi(f_focus->value);
			freq -= ((x - last_mouse_x) *tuning_step)/4;	// slow this down a bit
			sprintf(buff, "%ld", freq);
			set_field("r1:freq", buff);
			}
			return 1;
			break;
		*/
	}
	return 0;	
}

void remote_execute(char *cmd){

	if (q_remote_commands.overflow)
		q_empty(&q_remote_commands);
	while (*cmd)
		q_write(&q_remote_commands, *cmd++);
	q_write(&q_remote_commands, 0);
}

static void call_wipe(void){
	contact_callsign[0] = 0;
	contact_grid[0] = 0;
	received_rst[0] = 0;
	sent_rst[0] = 0;
	set_field("#log_ed", "");
	update_log_ed();
}

static void update_log_ed(void){
	struct field *f = get_field("#log_ed");
	struct field *f_sent_exchange = get_field("#sent_exchange");

	char *log_info = f->label;

	strcpy(log_info, "Log:");
	if (strlen(contact_callsign))
		strcat(log_info, contact_callsign);
	else {
		f->is_dirty = 1;
		return;
	}

	if (strlen(contact_grid)){
		strcat(log_info, " Grid:");
		strcat(log_info, contact_grid);
	}

	strcat(log_info, " Sent:");
	if (strlen(sent_rst)){	
		strcat(log_info, sent_rst);
		if (strlen(f_sent_exchange->value)){
			strcat(log_info, " ");
			strcat(log_info, f_sent_exchange->value);
		}
	}
	else	
		strcat(log_info, "-");


	strcat(log_info, " My:");
	if (strlen(received_rst)){
		strcat(log_info, received_rst);
		if (strlen(received_exchange)){
			strcat(log_info, " ");
			strcat(log_info, received_exchange);
		}
	} else
		strcat(log_info, "-");

	f->is_dirty = 1;
}

int do_console(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	char buff[100], *p, *q;

	int line_height = font_table[f->font_index].height; 	
	int n_lines = (f->height / line_height) - 1;
	int	l = 0;

	switch(event){
		case FIELD_DRAW:
			draw_log(gfx, f);
			return 1;
		break;
		case GDK_BUTTON_PRESS:
			l = console_current_line - ((f->y + f->height - b)/line_height);
			if (l < 0)
				l += MAX_CONSOLE_LINES;
			console_selected_line = l;
		/*
			if (!strcmp(get_field("r1:mode")->value, "FT8")){
				char ft8_response[100];
				ft8_interpret(console_stream[l].text, ft8_response);
				if (ft8_response[0] != 0){
					//set_field("#text_in", ft8_response);
					update_log_ed();
				}
			}
		*/
			f->is_dirty = 1;
			return 1;
		break;
	}
	return 0;	
}

int do_status(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	char buff[100];

	if (event == FIELD_DRAW){
		time_t now = time(NULL);
		struct tm *tmp = gmtime(&now);

		sprintf(buff, "%s | %s", get_field("#mycallsign")->value, get_field("#mygrid")->value);
		draw_text(gfx, f->x+1, f->y+2 , buff, FONT_FIELD_LABEL);
		
		sprintf(buff, "%04d/%02d/%02d %02d:%02d:%02dZ",  
			tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec); 
		int width = measure_text(gfx, buff, FONT_FIELD_LABEL);
		int line_height = font_table[f->font_index].height; 	
		if (width < f->width)
			draw_text(gfx, f->x + f->width - width - 1, f->y + 2, buff, FONT_FIELD_LABEL);
		else
			draw_text(gfx, f->x,  f->y + 2 + line_height, buff, FONT_FIELD_LABEL);

		strcpy(f->value, buff);
		f->is_dirty = 1;
		f->update_remote = 1;
		return 1;
	}
	return 0;
}

static void execute_app(char *app){
	char buff[1000];

	sprintf(buff, "%s 0> /dev/null", app); 
	int pid = fork();
	if (!pid){
		system(buff);
		exit(0);	
	}
}

int do_text(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	int width, offset, text_length, line_start, y;	
	char this_line[MAX_FIELD_LENGTH];
	int text_line_width = 0;

	if (event == FIELD_EDIT){
		// if it is a command, then execute it and clear the field
		if (f->value[0] == COMMAND_ESCAPE &&  strlen(f->value) > 1 && (a == '\n' || a == MIN_KEY_ENTER)){
			cmd_exec(f->value + 1);
			f->value[0] = 0;
			update_field(f);
		} else if ((a =='\n' || a == MIN_KEY_ENTER) && !strcmp(get_field(R1_MODE)->value, "FT8") 
			&& f->value[0] != COMMAND_ESCAPE){
			ft8_tx(f->value, atoi(get_field(_TX_PITCH)->value));
			f->value[0] = 0;		
		} else if (a >= ' ' && a <= 127 && strlen(f->value) < f->max-1){
			int l = strlen(f->value);
			f->value[l++] = a;
			f->value[l] = 0;
		} else if ((a == MIN_KEY_BACKSPACE || a == 8) && strlen(f->value) > 0){
			// handle ascii delete 8 or gtk 
			int l = strlen(f->value) - 1;
			f->value[l] = 0;
		}
		f->is_dirty = 1;
		f->update_remote = 1;
		return 1;
	} else if (event == FIELD_DRAW){

		fill_rect(gfx, f->x, f->y, f->width,f->height, COLOR_BACKGROUND);
		text_length = strlen(f->value);
		line_start = 0;
		y = f->y + 2;
		text_line_width = 0;

		while(text_length > 0){
			if (text_length > console_cols){
				strncpy(this_line, f->value + line_start, console_cols);
				this_line[console_cols] = 0;
			} else
				strcpy(this_line, f->value + line_start);		
			draw_text(gfx, f->x + 2, y, this_line, f->font_index);
			text_line_width= measure_text(gfx, this_line, f->font_index);
			y += 14;
			line_start += console_cols;
			text_length -= console_cols;
		}
		// draw the text cursor, if there is no text, the text baseline is zero
		if (strlen(f->value))
			y -= 14;
		fill_rect(gfx, f->x + text_line_width + 5, y+3, 9, 10, f->font_index);
		return 1;
	}
	return 0;
}


int do_pitch(struct field *f, cairo_t *gfx, int event, int a, int b, int c){

	int	v = atoi(f->value);

	if (event == FIELD_EDIT){
		if (a == MIN_KEY_UP && v + f->step <= f->max){
			v += f->step;
		} else if (a == MIN_KEY_DOWN && v - f->step >= f->min){
			v -= f->step;
		}
		sprintf(f->value, "%d", v);
		update_field(f);
		modem_set_pitch(v);
		char buff[20], response[20];
		sprintf(buff, "rx_pitch=%d", v);
		sdr_request(buff, response);

		// move the bandwidth accordingly
  		int mode = mode_id(get_field(R1_MODE)->value);
		int bw = 4000;
		switch(mode){
			case MODE_CW:
			case MODE_CWR:
				bw = atoi(get_field(_BW_CW)->value);
				set_bandwidth(bw);
				break;
			case MODE_DIGITAL:
				bw = atoi(get_field(_BW_DIGITAL)->value);
				set_bandwidth(bw);
				break;
		}
		return 1;
	}
		
	return 0;
}

// called for RIT as well as the main tuning
int do_tuning(struct field *f, cairo_t *gfx, int event, int a, int b, int c){

	static struct timespec last_change_time, this_change_time;

	int	v = atoi(f->value);
	int temp_tuning_step = tuning_step;

	if (event == FIELD_EDIT){

		if (!strcmp(get_field(TUNING_ACCELARATION)->value, "ON")){
    		clock_gettime(CLOCK_MONOTONIC_RAW, &this_change_time);
   			uint64_t delta_us = (this_change_time.tv_sec - last_change_time.tv_sec) * 1000000 + (this_change_time.tv_nsec - last_change_time.tv_nsec) / 1000;
    		char temp_char[100];
    		// sprintf(temp_char, "delta: %d", delta_us);
    		// strcat(temp_char,"\r\n");
    		// write_console(FONT_LOG, temp_char);
   			clock_gettime(CLOCK_MONOTONIC_RAW, &last_change_time);
    		if (delta_us < atof(get_field(TUNING_ACCEL_THRESH2)->value)){
    			if (tuning_step < 10000){
       				tuning_step = tuning_step * 100;
        			// sprintf(temp_char, "x100 activated\r\n");
        			// write_console(FONT_LOG, temp_char);
				}
    		} else if (delta_us < atof(get_field(TUNING_ACCEL_THRESH1)->value)){
    			if (tuning_step < 1000){
    				tuning_step = tuning_step * 10;
        			// printf(temp_char, "x10 activated\r\n");
        			// write_console(FONT_LOG, temp_char);
    			}
  			}
		}

		if (a == MIN_KEY_UP && v + f->step <= f->max){
			// this is tuning the radio
			if (!strcmp(get_field(_RIT)->value, "ON")){
				struct field *f = get_field(_RIT_DELTA);
				int rit_delta = atoi(f->value);
				if(rit_delta < MAX_RIT){
					rit_delta += tuning_step;
					char tempstr[100];
					sprintf(tempstr, "%d", rit_delta);
					set_field(_RIT_DELTA, tempstr);
					// printf("moved rit to %s\n", f->value);
				} else
					return 1;
			} else
				v = (v / tuning_step + 1)*tuning_step;
		} else if (a == MIN_KEY_DOWN && v - f->step >= f->min){
			if (!strcmp(get_field(_RIT)->value, "ON")){
				struct field *f = get_field(_RIT_DELTA);
				int rit_delta = atoi(f->value);
				if (rit_delta > -MAX_RIT){
					rit_delta -= tuning_step;
					char tempstr[100];
					sprintf(tempstr, "%d", rit_delta);
					set_field(_RIT_DELTA, tempstr);
					// printf("moved rit to %s\n", f->value);
				}
				else
					return 1;
			}
			else
				v = (v / tuning_step - 1)*tuning_step;
			abort_tx();
		}
		
		sprintf(f->value, "%d",  v);
		tuning_step = temp_tuning_step;
		// send the new frequency to the sbitx core
		char buff[100];
		sprintf(buff, "%s=%s", f->cmd, f->value);
		do_cmd(buff);
		// update the GUI
		update_field(f);
		settings_updated++;
		// leave it to us, we have handled it

		return 1;
	} else if (event == FIELD_DRAW){
		draw_dial(f, gfx);

		return 1; 
	}
	return 0;	
}

int do_kbd(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	if(event == GDK_BUTTON_PRESS){
		struct field *f_text = get_field(_TEXT_IN);
		if (!strcmp(f->cmd, _KBD_MACRO))
			set_ui(LAYOUT_MACROS);
		else if (!strcmp(f->cmd, _KBD_BS))
			edit_field(f_text, MIN_KEY_BACKSPACE);
		else if (!strcmp(f->cmd, _KBD_ENTER))
			edit_field(f_text, '\n');
		else
			edit_field(f_text, f->value[0]);
		focus_since = millis();
		return 1;
	}	
	return 0;
}

static void write_call_log(){
	struct field *f_sent_exchange = get_field(_SENT_EXCHANGE);

	logbook_add(contact_callsign, sent_rst, f_sent_exchange->value, received_rst, received_exchange); 
	write_console(FONT_LOG, "\nQSO logged with ");
	write_console(FONT_LOG, contact_callsign);
	write_console(FONT_LOG, " ");
	write_console(FONT_LOG, f_sent_exchange->value); 
	write_console(FONT_LOG, "\n");

	struct field *f_contest_serial = get_field(_CONTEST_SERIAL);
	int  ctx_serial = atoi(f_contest_serial->value);
	if (ctx_serial > 0){
		ctx_serial++;
		char sent[100], serial[100];

		sprintf(sent, "%04d", ctx_serial);
		set_field(_SENT_EXCHANGE, sent);
		sprintf(serial, "%d", ctx_serial);
		set_field(_CONTEST_SERIAL, serial);	
	}
	// wipe it clean, deffered for the time being
	// call_wipe();
}


// this interprets the log being filled in bits and pieces in the following order
// callsign, sent rst, received rst and exchange
static void interpret_log(char *text){
	int i, j;
	char *p, *q;
	int mode = mode_id(get_field(R1_MODE)->value);

	p = text;
	while(*p == ' ')
		p++;
	
	if (contact_callsign[0] == 0){
		for (i = 0; *p && i < sizeof(contact_callsign) && *p > ' '; i++)
			contact_callsign[i] = *p++;
		contact_callsign[i] = 0;
	}

	while(*p == ' ')
		p++;

	if (sent_rst[0] == 0){
		//the first must be something between 1 and 5
		if ((mode == MODE_CW || mode == MODE_CWR) && p[0] >= '1' && p[0] <= '5'){
			sent_rst[0] = p[0];
			sent_rst[1] = p[1];
			sent_rst[2] = p[2];
			sent_rst[3] = 0;
			p += 3;
		} else if (p[0] >= '1' && p[0] <= '5' && (toupper(p[1]) == 'N' || isdigit(p[1]))){ //none cw modes
			sent_rst[0] = p[0];
			sent_rst[1] = p[1];
			sent_rst[2] = 0; 
			p += 2;
		}
	}

	while(*p == ' ')
		p++;

	if (received_rst[0] == 0){
		// the first must be something between 1 and 5
		if ((mode == MODE_CW|| mode == MODE_CWR) && p[0] >= '1' && p[0] <= '5'){
			received_rst[0] = p[0];
			received_rst[1] = p[1];
			received_rst[2] = p[2];
			received_rst[3] = 0;
			p += 3;
		} 
		else if (p[0] >= '1' && p[0] <= '5' && (toupper(p[1]) == 'N' || isdigit(p[1]))){ //none cw modes
			received_rst[0] = p[0];
			received_rst[1] = p[1];
			received_rst[2] = 0; 
			p += 2;
		}
	}

	while(*p == ' ')
		p++;

	// the rest is exchange received
	for (i = 0; i < sizeof(received_exchange) && *p > ' '; i++)
		received_exchange[i] = *p++;

	received_exchange[i] = 0;
}

void macro_get_var(char *var, char *s){
	*s = 0;

	if(!strcmp(var, "MYCALL"))
		strcpy(s, get_field(_MYCALLSIGN)->value);
	else if (!strcmp(var, "CALL"))
		strcpy(s, contact_callsign);
	else if (!strcmp(var, "SENTRST"))
		sprintf(s, "%s", sent_rst);
	else if (!strcmp(var, "GRID"))
		strcpy(s, get_field(_MYGRID)->value);
	else if (!strcmp(var, "GRIDSQUARE")){
		strcpy(var, get_field(_MYGRID)->value);
		var[4] = 0;
		strcpy(s, var);
	}
	else if (!strcmp(var, "EXCH")){
		strcpy(s, get_field(_SENT_EXCHANGE)->value);
	}
	else if (!strcmp(var, "WIPE"))
		call_wipe();
	else if (!strcmp(var, "LOG")){
		write_call_log();
	} else
		*s = 0;
}

static void open_url(char *url){
	char temp_line[200];

	sprintf(temp_line, "chromium-browser --log-leve=3 "
	"--enable-features=OverlayScrollbar %s"
	"  >/dev/null 2> /dev/null &", url);
	execute_app(temp_line);
}

static void qrz(char *callsign){
	char url[1000];

	sprintf(url, "https://qrz.com/DB/%s &", callsign);
	open_url(url);
}

int do_macro(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	char buff[256], *mode;

	if(event == GDK_BUTTON_PRESS){
		int fn_key = atoi(f->cmd+3); // skip past the '#mf' and read the function key number

		if (!strcmp(f->cmd, _MFKBD)){
			set_ui(LAYOUT_KBD);
			return 1;
		} else if (!strcmp(f->cmd, _MFQRZ) && strlen(contact_callsign) > 0){
			qrz(contact_callsign);
			return 1;
		} else if (!strcmp(f->cmd, _MFWIPE)){
			call_wipe();
			return 1;
		} else if (!strcmp(f->cmd, _MFLOG)){
			write_call_log();
			return 1;
		} else 
		 	macro_exec(fn_key, buff);
	
		mode = get_field(R1_MODE)->value;

		// add the end of transmission to the expanded buffer for the fldigi modes
		if (!strcmp(mode, "RTTY") || !strcmp(mode, "PSK31")){
			strcat(buff, "^r");
			tx_on(TX_SOFT);
		}

		if (!strcmp(mode, "FT8") && strlen(buff)){
			ft8_tx(buff, atoi(get_field(_TX_PITCH)->value));
			set_field(_TEXT_IN, "");
			// write_console(FONT_LOG_TX, buff);
		} else if (strlen(buff)){
			set_field(_TEXT_IN, buff);
			// put it in the text buffer and hope it gets transmitted!
		}
		return 1;
	} else if (event == FIELD_DRAW){
		int width, offset, text_length, line_start, y;	
		char this_line[MAX_FIELD_LENGTH];
		int text_line_width = 0;

		fill_rect(gfx, f->x, f->y, f->width,f->height, COLOR_BACKGROUND);
		rect(gfx, f->x, f->y, f->width,f->height, COLOR_CONTROL_BOX, 1);

		width = measure_text(gfx, f->label, FONT_FIELD_LABEL);
		offset = f->width/2 - width/2;
		if (strlen(f->value) == 0)
			draw_text(gfx, f->x +5, f->y+13 , f->label , FONT_FIELD_LABEL);
		else {
			if (strlen(f->label)){
				draw_text(gfx, f->x+5, f->y+5 ,  f->label, FONT_FIELD_LABEL);
				draw_text(gfx, f->x+5 , f->y+f->height - 20 , f->value , f->font_index);
			} else
				draw_text(gfx, f->x+offset , f->y+5, f->value , f->font_index);
			}	
		return 1;
	}

	return 0;
}

int do_record(struct field *f, cairo_t *gfx, int event, int a, int b, int c){
	if (event == FIELD_DRAW){

		if (f_focus == f)
			rect(gfx, f->x, f->y, f->width-1,f->height, COLOR_SELECTED_BOX, 2);
		else if (f_hover == f)
			rect(gfx, f->x, f->y, f->width,f->height, COLOR_SELECTED_BOX, 1);
		else 
			rect(gfx, f->x, f->y, f->width,f->height, COLOR_CONTROL_BOX, 1);

		int width = measure_text(gfx, f->label, FONT_FIELD_LABEL);
		int offset = f->width/2 - width/2;
		int	label_y = f->y + ((f->height 
			- font_table[FONT_FIELD_LABEL].height - 5  
			- font_table[FONT_FIELD_VALUE].height)/2);
		draw_text(gfx, f->x + offset, label_y, f->label, FONT_FIELD_LABEL);


		char duration[12];
		label_y += font_table[FONT_FIELD_LABEL].height;

		if (record_start){
			width = measure_text(gfx, f->value, f->font_index);
			offset = f->width/2 - width/2;
			time_t duration_seconds = time(NULL) - record_start;
			int minutes = duration_seconds/60;
			int seconds = duration_seconds % 60;
			sprintf(duration, "%d:%02d", minutes, seconds); 	
		} else
			strcpy(duration, "OFF");
		width = measure_text(gfx, duration, FONT_FIELD_VALUE);
		draw_text(gfx, f->x + f->width/2 - width/2, label_y, duration, f->font_index);
		return 1;
	}
	return 0;
}

void tx_on(int trigger){
	char response[100];

	if (trigger != TX_SOFT && trigger != TX_PTT){
		puts("Error: tx_on trigger should be SOFT or PTT");
		return;
	}

	struct field *f = get_field(R1_MODE);
	if (f){
		if (!strcmp(f->value, "CW"))
			tx_mode = MODE_CW;
		else if (!strcmp(f->value, "CWR"))
			tx_mode = MODE_CWR;
		else if (!strcmp(f->value, "USB"))
			tx_mode = MODE_USB;
		else if (!strcmp(f->value, "LSB"))
			tx_mode = MODE_LSB;
		else if (!strcmp(f->value, "NBFM"))
			tx_mode = MODE_NBFM;
		else if (!strcmp(f->value, "AM"))
			tx_mode = MODE_AM;
		else if (!strcmp(f->value, "2TONE"))
			tx_mode = MODE_2TONE;
		else if (!strcmp(f->value, "DIGITAL"))
			tx_mode = MODE_DIGITAL;
	}

	if (in_tx == 0){
		sdr_request("tx=on", response);	
		in_tx = trigger; //can be PTT or softswitch
		char response[20];
		struct field *freq = get_field("r1:freq");
		set_operating_freq(atoi(freq->value), response);
		update_field(get_field("r1:freq"));
		printf("TX\n");
	}

	tx_start_time = millis();
}

void tx_off(){
	char response[100];

	modem_abort();

	if (in_tx){
		sdr_request("tx=off", response);	
		in_tx = 0;
		sdr_request("key=up", response);
		char response[20];
		struct field *freq = get_field(R1_FREQ);
		set_operating_freq(atoi(freq->value), response);
		update_field(get_field(R1_FREQ));
		printf("RX\n");
	}
	sound_input(0); // it is a low overhead call, might as well be sure
}


static int layout_handler(void* user, const char* section, 
	const char* name, const char* value)
{
	// the section is the field's name

	printf("setting %s:%s to %d\n", section, name, atoi(value));
	struct field *f = get_field(section);
	if (!strcmp(name, "x"))
		f->x = atoi(value);
	else if (!strcmp(name, "y"))
		f->y = atoi(value);
	else if (!strcmp(name, "width"))
		f->width = atoi(value);
	else if (!strcmp(name, "height"))
		f->height = atoi(value);	
}

static void set_ui(int id){
	struct field *f = get_field(_KBD_Q);

	if (id == LAYOUT_KBD){
		// the "#kbd" is out of screen, get it up and "#mf" down
		for (int i = 0; active_layout[i].cmd[0] > 0; i++){
			if (!strncmp(active_layout[i].cmd, "#kbd", 4) && active_layout[i].y > 1000)
				active_layout[i].y -= 1000;
			else if (!strncmp(active_layout[i].cmd, "#mf", 3) && active_layout[i].y < 1000)
				active_layout[i].y += 1000;
			active_layout[i].is_dirty = 1;	
		}
	}
	if (id == LAYOUT_MACROS) {
		// the "#mf" is out of screen, get it up and "#kbd" down
		for (int i = 0; active_layout[i].cmd[0] > 0; i++){
			if (!strncmp(active_layout[i].cmd, "#kbd", 4) && active_layout[i].y < 1000)
				active_layout[i].y += 1000;
			else if (!strncmp(active_layout[i].cmd, "#mf", 3) && active_layout[i].y > 1000)
				active_layout[i].y -= 1000;
			active_layout[i].is_dirty = 1;	
		}
	}
	current_layout = id;
}

int static cw_keydown = 0;
int	static cw_hold_until = 0;
int static cw_hold_duration = 150;

static void cw_key(int state){
	char response[100];
	if (state == 1 && cw_keydown == 0){
		sdr_request("key=down", response);
		cw_keydown = 1;
	}
	else if (state == 0 && cw_keydown == 1){
		cw_keydown = 0;
	}
	// printf("cw key = %d\n", cw_keydown);
}


static int control_down = 0;
static gboolean on_key_release (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	key_modifier = 0;

	if (event->keyval == MIN_KEY_CONTROL){
		control_down = 0;
	}

	if (event->keyval == MIN_KEY_TAB){
		tx_off();
	}

}

static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	char request[1000], response[1000];

	if (event->keyval == MIN_KEY_CONTROL){
		control_down = 1;
	}

	if (control_down){
		GtkClipboard *clip;
		struct field *f;	
		switch(event->keyval){
			case 'r':
				tx_off();
				break;
			case 't':
				tx_on(TX_SOFT);
				break;
			case 'm':
				if (current_layout == LAYOUT_MACROS)
					set_ui(LAYOUT_KBD);
				else
					set_ui(LAYOUT_MACROS);
				break;
			case 'q':
				tx_off();
				set_field(_RECORD, "OFF");
				save_user_settings(1);
				exit(0);
				break;
			case 'c':
				f = get_field(_TEXT_IN);
				clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				gtk_clipboard_set_text(clip, f->value, strlen(f->value));
				break; 
			case 'l':
				write_call_log();
				break;
			case 'v':
				clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				if (clip){
					int i = 0;
					gchar *text = gtk_clipboard_wait_for_text(clip);
					f = get_field(_TEXT_IN);
					if (text){
						i = strlen(f->value);
						while(i < MAX_FIELD_LENGTH-1 && *text){
							if (*text >= ' ' || *text == '\n' || 
								(*text >= ' ' && *text <= 128))
								f->value[i++] = *text;  
							text++;	
						}
						f->value[i] = 0;
						update_field(f);
					}
				}
				break;
		}
		return FALSE;
	}

	if (f_focus && f_focus->value_type == FIELD_TEXT){
		edit_field(f_focus, event->keyval); 
		return FALSE;
	}
		
	// printf("keyPress %x %x\n", event->keyval, event->state);
	// key_modifier = event->keyval;
	switch(event->keyval){
		case MIN_KEY_ESC:
			modem_abort();
			tx_off();
			call_wipe();
			update_log_ed();
			break;
		case MIN_KEY_UP:
			if (f_focus == NULL && f_hover > active_layout){
				hover_field(f_hover - 1);
				// printf("Up, hover %s\n", f_hover->cmd);
			}else if (f_focus){
				edit_field(f_focus, MIN_KEY_UP);
			}
			break;
		case MIN_KEY_DOWN:
			if (f_focus == NULL && f_hover && strcmp(f_hover->cmd, "")){
				hover_field(f_hover + 1);
				// printf("Down, hover %d\n", f_hover);
			}
			else if (f_focus){
				edit_field(f_focus, MIN_KEY_DOWN);
			}
			break;
		case MIN_KEY_CONTROL:
			key_modifier |= event->keyval;
			// printf("key_modifier set to %d\n", key_modifier);
			break;
		default:
			// by default, all text goes to the text_input control
			if (event->keyval == MIN_KEY_ENTER)
				edit_field(get_field(_TEXT_IN), '\n');
			else if (MIN_KEY_F1 <= event->keyval && event->keyval <= MIN_KEY_F12){
				int fn_key = event->keyval - MIN_KEY_F1 + 1;
				char fname[10];
				sprintf(fname, "#mf%d", fn_key);
				do_macro(get_field(fname), NULL, GDK_BUTTON_PRESS, 0, 0, 0);
			} else
				edit_field(get_field(_TEXT_IN), event->keyval);
			// if (f_focus)
			// 	edit_field(f_focus, event->keyval); 
			// printf("key = %d (%c)\n", event->keyval, (char)event->keyval); 	
	}
	return FALSE; 
}

static gboolean on_scroll (GtkWidget *widget, GdkEventScroll *event, gpointer data) {
	
	if (f_focus){
		if (event->direction == 0){
    		if (!strcmp(get_field(REVERSE_SCROLLING)->value, "ON")){
	  			edit_field(f_focus, MIN_KEY_DOWN);
    		} else {
		  		edit_field(f_focus, MIN_KEY_UP);
    		}
		} else {
    		if (!strcmp(get_field(REVERSE_SCROLLING)->value, "ON")){
				edit_field(f_focus, MIN_KEY_UP);
    		} else {
				edit_field(f_focus, MIN_KEY_DOWN);
    		}
		}
	}	
}


static gboolean on_window_state (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	mouse_down = 0;
}

static gboolean on_mouse_release (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	struct field *f;

	mouse_down = 0;
	if (event->type == GDK_BUTTON_RELEASE && event->button == GDK_BUTTON_PRIMARY){
		// printf("mouse release at %d, %d\n", (int)(event->x), (int)(event->y));
	}
	/* We've handled the event, stop processing */
	return TRUE;
}


static gboolean on_mouse_move (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	char buff[100];

	if (!mouse_down)
		return false;

	int x = (int)(event->x);
	int y = (int)(event->y);

	// if a control is in focus and it handles the mouse drag, then just do that
	// else treat it as a spin up/down of the control
	if (f_focus){

		if (!f_focus->fn ||  !f_focus->fn(f_focus, NULL, GDK_MOTION_NOTIFY, event->x, event->y, 0)){
			// just emit up or down
			if(last_mouse_x < x || last_mouse_y > y)
				edit_field(f_focus, MIN_KEY_UP);
			else if (last_mouse_x > x || last_mouse_y < y)
				edit_field(f_focus, MIN_KEY_DOWN);
		}
	}
	last_mouse_x = x;
	last_mouse_y = y;

	return true;
}

static gboolean on_mouse_press (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	struct field *f;

	if (event->type == GDK_BUTTON_RELEASE){
		mouse_down = 0;
		// puts("mouse up in on_mouse_press");
	}
	else if (event->type == GDK_BUTTON_PRESS /*&& event->button == GDK_BUTTON_PRIMARY*/){

		// printf("mouse event at %d, %d\n", (int)(event->x), (int)(event->y));
		for (int i = 0; active_layout[i].cmd[0] > 0; i++) {
			f = active_layout + i;
			if (f->x < event->x && event->x < f->x + f->width 
					&& f->y < event->y && event->y < f->y + f->height){
				focus_field(f);
				if (f->fn){
					// we get out of the loop just to prevent two buttons from responding
					if (f->fn(f, NULL, GDK_BUTTON_PRESS, event->x, event->y, event->button))
						break;	
				}
			} 
		}
		last_mouse_x = (int)event->x;
		last_mouse_y = (int)event->y;
		mouse_down = 1;
	}
	/* We've handled the event, stop processing */
	return FALSE;
}


/*
Turns out (after two days of debugging) that GTK is not thread-safe and
we cannot invalidate the spectrum from another thread .
This redraw is called from another thread. Hence, we set a flag here 
that is read by a timer tick from the main UI thread and the window
is posted a redraw signal that in turn triggers the redraw_all routine.
Don't ask me, I only work around here.
*/

static void redraw(){
	struct field *f;
	f = get_field(_CONSOLE);
	f->is_dirty = 1;
	f = get_field(_TEXT_IN);
	f->is_dirty = 1;
}

/* hardware specific routines */


int key_poll(){
	int key = CW_IDLE;
	int input_method = get_cw_input_method();

	if (input_method == CW_IAMBIC || input_method == CW_IAMBICB){	
		if (digitalRead(PTT) == LOW)
			key |= CW_DASH;
		if (digitalRead(DASH) == LOW)
			key |= CW_DOT;
	}
	//straight key
	else if (digitalRead(PTT) == LOW || digitalRead(DASH) == LOW)
			key = CW_DOWN;

	//printf("key %d\n", key);
	return key;
}

static void enc_init(struct encoder *e, int speed, int pin_a, int pin_b){
	e->pin_a = pin_a;
	e->pin_b = pin_b;
	e->speed = speed;
	e->history = 5;
}

static int enc_state (struct encoder *e) {
	return (digitalRead(e->pin_a) ? 1 : 0) + (digitalRead(e->pin_b) ? 2: 0);
}

static int enc_read(struct encoder *e) {
	int result = 0; 
	int newState;
  
	newState = enc_state(e); // Get current state  
    
	if (newState != e->prev_state)
    	delay (1);
  
	if (enc_state(e) != newState || newState == e->prev_state)
    	return 0; 

	// these transitions point to the encoder being rotated anti-clockwise
	if ((e->prev_state == 0 && newState == 2) || 
    	(e->prev_state == 2 && newState == 3) || 
    	(e->prev_state == 3 && newState == 1) || 
    	(e->prev_state == 1 && newState == 0)){
    	e->history--;
    	//result = -1;
    }
	// these transitions point to the enccoder being rotated clockwise
	if ((e->prev_state == 0 && newState == 1) || 
    	(e->prev_state == 1 && newState == 3) || 
    	(e->prev_state == 3 && newState == 2) || 
    	(e->prev_state == 2 && newState == 0)){
    	e->history++;
    }
	e->prev_state = newState; // Record state for next pulse interpretation
	if (e->history > e->speed){
    	result = 1;
   		e->history = 0;
	}
	if (e->history < -e->speed){
    	result = -1;
    	e->history = 0;
	}
	return result;
}

static int tuning_ticks = 0;
static void tuning_isr(void){
	int tuning = enc_read(&enc_b);
	if (tuning < 0)
		tuning_ticks++;
	if (tuning > 0)
		tuning_ticks--;	
}

static void query_swr(){
	uint8_t response[4];
	int16_t vfwd, vref;
	int  vswr;
	char buff[20];

	if (!in_tx)
		return;
	if (i2cbb_read_i2c_block_data(0x8, 0, 4, response) == -1)
		return;

	vfwd = vref = 0;

	memcpy(&vfwd, response, 2);
	memcpy(&vref, response+2, 2);
	if (vref >= vfwd)
		vswr = 100;
	else
		vswr = (10*(vfwd + vref))/(vfwd-vref);
	sprintf(buff,"%d", (vfwd * 40)/68);
	set_field(_FWDPOWER, buff);		
	sprintf(buff, "%d", vswr);
	set_field(_VSWR, buff);
}


static void init_gpio_pins(void) {
    #if 0
    char directory[PATH_MAX];
    realpath(argv_0, directory);
    strcpy(directory, dirname(directory));
    strcat(directory, "/../setGPIO.py");
    system(directory);
    #else
    // this requires wiringpi 2.61 (unoffical mods)
    const int8_t PINS_IN[] = {7, 0, 2, 3, 12, 13, 14, 21, -1};
    const int8_t PINS_OUT[] = {4, 5, 6, 10, 11, 27, -1};

    int8_t *i = (int8_t *) PINS_IN;
    while (*i >= 0) {
        pinMode(*i, INPUT);
        pullUpDnControl(*i, PUD_UP);
        i++;
    }

    i = (uint8_t *) PINS_OUT;
    while (*i >= 0) {
        pinMode(*i, OUTPUT);
        digitalWrite(*i, LOW);
        i++;
    }
    #endif
}

static void hw_init(){
	wiringPiSetup();
	init_gpio_pins();

	enc_init(&enc_a, ENC_FAST, ENC1_B, ENC1_A);
	enc_init(&enc_b, ENC_FAST, ENC2_A, ENC2_B);

	int e = g_timeout_add(1, ui_tick, NULL);

	wiringPiISR(ENC2_A, INT_EDGE_BOTH, tuning_isr);
	wiringPiISR(ENC2_B, INT_EDGE_BOTH, tuning_isr);
}

void hamlib_tx(int tx_input){
	if (tx_input){
    	sound_input(1);
		tx_on(TX_SOFT);
	} else {
    	sound_input(0);
		tx_off();
	}
}

int get_cw_delay(){
	return atoi(get_field(_CWDELAY)->value);
}

int get_cw_input_method(){
	struct field *f = get_field(_CWINPUT);
	if (!strcmp(f->value, "KEYBOARD"))
		return CW_KBD;
	else if (!strcmp(f->value, "IAMBIC"))
		return CW_IAMBIC;
	else if (!strcmp(f->value, "IAMBICB"))
		return CW_IAMBICB;
	else
		return CW_STRAIGHT;
}

int get_pitch(){
	struct field *f = get_field(RX_PITCH);
	return atoi(f->value);
}

int get_cw_tx_pitch(){
	struct field *f = get_field(_TX_PITCH);
	return atoi(f->value);
}

int get_data_delay(){
	return data_delay;
}

int get_wpm(){
	struct field *f = get_field(_TX_WPM);
	return atoi(f->value);
}

long get_freq(){
	return atol(get_field(R1_FREQ)->value);
}

static void set_bandwidth(int hz){
	char buff[10], bw_str[10];
	int low, high;
	struct field *f_mode = get_field(R1_MODE);
	struct field *f_pitch = get_field(RX_PITCH);

	switch(mode_id(f_mode->value)){
		case MODE_CW:
		case MODE_CWR:
			low = atoi(f_pitch->value) - hz/2;
			high = atoi(f_pitch->value) + hz/2;
			sprintf(bw_str, "%d", high - low);
			set_field(_BW_CW, bw_str);
			break;
		case MODE_LSB:
		case MODE_USB:
			low = 300;
			high = low + hz;
			sprintf(bw_str, "%d", high - low);
			set_field(_BW_VOICE, bw_str);
			break;
		case MODE_DIGITAL:
			low = atoi(f_pitch->value) - (hz/2);
			high = atoi(f_pitch->value) + (hz/2);
			sprintf(bw_str, "%d", high - low);
			set_field(_BW_DIGITAL, bw_str);
			break;
		case MODE_FT8:
			low = 50;
			high = 4000;
			break;
		default:
			low = 50;
			high = 3000;
	}

	if (low < 50)
		low = 50;
	if (high > 5000)
		high = 5000;

	//now set the bandwidth
	sprintf(buff, "%d", low);
	set_field(R1_LOW, buff);
	sprintf(buff, "%d", high);
	set_field(R1_HIGH, buff);
}

static void set_mode(char *mode){
	struct field *f = get_field(R1_MODE);
	char umode[10], bw_str[10];
	int i;

	for (i = 0; i < sizeof(umode) - 1 && *mode; i++)
		umode[i] = toupper(*mode++);
	umode[i] = 0;

	if(!set_field(R1_MODE, umode)){
		int new_bandwidth = 3000;
        int m_id = mode_id(f->value);
	
		switch(m_id){
			case MODE_CW:
			case MODE_CWR:
				new_bandwidth = atoi(get_field(_BW_CW)->value);
				break;
			case MODE_USB:
			case MODE_LSB:
				new_bandwidth = atoi(get_field(_BW_VOICE)->value);
				break;
			case MODE_DIGITAL:
				new_bandwidth = atoi(get_field(_BW_DIGITAL)->value);
				break;
			case MODE_FT8:
				new_bandwidth = 4000;
				break;
		}
		set_bandwidth(new_bandwidth);
        int new_bfo = (m_id == MODE_LSB || m_id == MODE_CWR)
            ? LO_LSB : LO_USB;
        si5351bx_setfreq(1, new_bfo);
	}
	else
		write_console(FONT_LOG, "%s is not a mode\n");
}

static void get_mode(char *mode){
	struct field *f = get_field(R1_MODE);
	strcpy(mode, f->value);
}


static void bin_dump(int length, uint8_t *data){
	printf("i2c: ");
	for (int i = 0; i < length; i++)
		printf("%x ", data[i]);
	printf("\n");
}



static gboolean ui_tick(gpointer gook){
	int static ticks = 0;

	ticks++;

	while (q_length(&q_remote_commands) > 0){
		// read each command until the 
		char remote_cmd[1000];
		int c, i;
		for (i = 0; i < sizeof(remote_cmd)-2 &&  (c = q_read(&q_remote_commands)) > 0; i++){
			remote_cmd[i] = c;
		}
		remote_cmd[i] = 0;

		//echo the keystrokes for chatty modes like cw/rtty/psk31/etc
		if (!strncmp(remote_cmd, "key ", 4))
			for (int i = 4; remote_cmd[i] > 0; i++)
				edit_field(get_field(_TEXT_IN), remote_cmd[i]);	
		else {
			cmd_exec(remote_cmd);
			settings_updated = 1; //save the settings
		}
	}

	if (gtk_window_is_active(GTK_WINDOW(window)) == TRUE)
		for (struct field *f = active_layout; f->cmd[0] > 0; f++){
			if (f->is_dirty){
				if (f->y >= 0){
					GdkRectangle r;
					r.x = f->x;
					r.y = f->y;
					r.width = f->width;
					r.height = f->height;
					invalidate_rect(r.x, r.y, r.width, r.height);
				}
				// char buff[1000];
				// sprintf(buff, "%s %s", f->label, f->value);
				// printf("%d: updating dirty field %s\n",ticks,  f->cmd);
			}
		}
	// char message[100];
	
	// check the tuning knob
	struct field *f = get_field(R1_FREQ);

	if (abs(tuning_ticks) > 5)
		tuning_ticks *= 4;
	while (tuning_ticks > 0){
		edit_field(f, MIN_KEY_DOWN);
		tuning_ticks--;
    	// sprintf(message, "tune-\r\n");
    	// write_console(FONT_LOG, message);
	}
	while (tuning_ticks < 0){
		edit_field(f, MIN_KEY_UP);
		tuning_ticks++;
    	// sprintf(message, "tune+\r\n");
    	// write_console(FONT_LOG, message);
	}

	#ifndef N8ME
	if (ticks % 20 == 0){
  		modem_poll(mode_id(get_field(R1_MODE)->value));
	}
	#endif

	if (ticks == 100){

		char response[6], cmd[10];
		cmd[0] = 1;

		if(in_tx){
			char buff[10];

			sprintf(buff,"%d", fwdpower);
			set_field(_FWDPOWER, buff);		
			sprintf(buff, "%d", vswr);
			set_field(_VSWR, buff);
		}

		struct field *f = get_field(SPECTRUM);
		update_field(f);	// move this each time the spectrum watefall index is moved
		f = get_field(WATERFALL);
		update_field(f);
		f = get_field(_STATUS);
		update_field(f);

		if (digitalRead(ENC1_SW) == 0)
            focus_field(get_field(R1_VOLUME));

		if (record_start)
			update_field(get_field(_RECORD));

		// alternate character from the softkeyboard upon long press
		if (f_focus && focus_since + 500 < millis() 
				&& !strncmp(f_focus->cmd, "#kbd_", 5) && mouse_down){
			// emit the symbol
			struct field *f_text = get_field(_TEXT_IN);
			// replace the previous character with teh shifted one
			edit_field(f_text,MIN_KEY_BACKSPACE); 
			edit_field(f_text, f_focus->label[0]);
			focus_since = millis();
		}

		// check if low and high settings are stepping on each other
		char new_value[20];
		while (atoi(get_field(R1_LOW)->value) > atoi(get_field(R1_HIGH)->value)){
			sprintf(new_value, "%d", atoi(get_field(R1_HIGH)->value)+get_field(R1_HIGH)->step);
			set_field(R1_HIGH,new_value);
		}


		static char last_mouse_pointer_value[16];

		int cursor_type;

		if (strcmp(get_field(MOUSE_POINTER)->value, last_mouse_pointer_value)){
			sprintf(last_mouse_pointer_value,get_field(MOUSE_POINTER)->value);
			if (!strcmp(last_mouse_pointer_value,"BLANK")){
				cursor_type = GDK_BLANK_CURSOR;
			} else if (!strcmp(last_mouse_pointer_value,"RIGHT")){
				cursor_type = GDK_RIGHT_PTR;
			} else if (!strcmp(last_mouse_pointer_value,"CROSSHAIR")){
				cursor_type = GDK_CROSSHAIR;
			} else {
				cursor_type = GDK_LEFT_PTR;
			}
			GdkCursor* new_cursor;
			new_cursor = gdk_cursor_new_for_display (gdk_display_get_default(),cursor_type);
			gdk_window_set_cursor(gdk_get_default_root_window(), new_cursor);
		}

		ticks = 0;
	}
	// update_field(get_field(_TEXT_IN)); //modem might have extracted some text

	hamlib_slice();
	remote_slice();
	save_user_settings(0);

 
	f = get_field(R1_MODE);
	// straight key in CW
	if (f && (!strcmp(f->value, "2TONE") || !strcmp(f->value, "LSB") || !strcmp(f->value, "USB"))){
		if (digitalRead(PTT) == LOW && in_tx == 0)
			tx_on(TX_PTT);
		else if (digitalRead(PTT) == HIGH && in_tx  == TX_PTT)
			tx_off();
	}

	int scroll = enc_read(&enc_a);
	if (scroll && f_focus){
		if (scroll < 0)
			edit_field(f_focus, MIN_KEY_DOWN);
		else
			edit_field(f_focus, MIN_KEY_UP);
	}	
	return TRUE;
}

static void ui_init(int argc, char *argv[]){
 
	gtk_init( &argc, &argv );

	// we are using two deprecated functions here
	// if anyone has a hack around them, do submit it

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	#ifndef N8ME
		screen_width = gdk_screen_width();
		screen_height = gdk_screen_height();
	#endif
	#pragma pop

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// gtk_window_set_default_size(GTK_WINDOW(window), 800, 480);
	gtk_window_set_default_size(GTK_WINDOW(window), screen_width, screen_height);
	#ifdef N8ME
	// gtk_window_move(GTK_WINDOW(window), 0, 0);
	gtk_window_set_title(GTK_WINDOW(window), "sBITX-N8ME" );
	#else
	gtk_window_set_title(GTK_WINDOW(window), "sBITX" );
	#endif
	gtk_window_set_icon_from_file(GTK_WINDOW(window), "/home/pi/sbitx/sbitx_icon.png", NULL);

	display_area = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER(window), display_area );

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(display_area), "draw", G_CALLBACK(on_draw_event), NULL);
	g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(on_key_press), NULL);
	g_signal_connect(G_OBJECT(window), "key_release_event", G_CALLBACK(on_key_release), NULL);
	g_signal_connect(G_OBJECT(window), "window_state_event", G_CALLBACK(on_window_state), NULL);
	g_signal_connect(G_OBJECT(display_area), "button_press_event", G_CALLBACK(on_mouse_press), NULL);
	g_signal_connect(G_OBJECT(window), "button_release_event", G_CALLBACK(on_mouse_release), NULL);
	g_signal_connect(G_OBJECT(display_area), "motion_notify_event", G_CALLBACK(on_mouse_move), NULL);
	g_signal_connect(G_OBJECT(display_area), "scroll_event", G_CALLBACK(on_scroll), NULL);
	g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(on_resize), NULL);

	/* Ask to receive events the drawing area doesn't normally
	* subscribe to. In particular, we need to ask for the
	* button press and motion notify events that want to handle.
	*/
	gtk_widget_set_events (display_area, gtk_widget_get_events (display_area)
		| GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK 
		| GDK_SCROLL_MASK
		| GDK_POINTER_MOTION_MASK);

	// scale the fonts as needed, these need to be done just once
	for (int i = 0; i < FONT_COUNT; i++)
		font_table[i].height = (font_table[i].height * screen_height)/480;
	scale_ui();	
	gtk_widget_show_all(window);
//	if ((argc > 1) && (argv[1][0] == 'f'))
	    gtk_window_fullscreen(GTK_WINDOW(window));
	// else
	//     gtk_window_get_default_widget(GTK_WINDOW(window));

	focus_field(get_field(R1_VOLUME));
}

/* handle modem callbacks for more data */


int get_tx_data_byte(char *c){
	// take out the first byte and return it to the modem
	struct field *f = get_field("#text_in");
	int length = strlen(f->value);

	if (f->value[0] == '\\' || !length)
		return 0;
	if (length){
		*c = f->value[0];
		// now shift the buffer down, hopefully, this copies the trailing null too
		for (int i = 0; i < length; i++)
			f->value[i] = f->value[i+1];
	}
	f->is_dirty = 1;
	f->update_remote = 1;
	// update_field(f);
	return length;
}

int get_tx_data_length(){
	struct field *f = get_field(_TEXT_IN);

	if (strlen(f->value) == 0)
		return 0;

	if (f->value[0] != COMMAND_ESCAPE)
		return strlen(get_field(_TEXT_IN)->value);
	else
		return 0;
}

/* handle the ui request and update the controls */

static void change_band(char *request){
	int i, old_band, new_band; 
	int max_bands = sizeof(band_stack)/sizeof(struct band);
	long new_freq, old_freq;
	char buff[100];

	// find the band that has just been selected, the first char is #, we skip it
	for (new_band = 0; new_band < max_bands; new_band++)
		if (!strcmp(request, band_stack[new_band].name))
			break;

	// continue if the band is legit
	if (new_band == max_bands)
		return;

	// find out the tuned frequency
	struct field *f = get_field(R1_FREQ);
	old_freq = atol(f->value);
	f = get_field(R1_MODE);
	int old_mode = mode_id(f->value);
	if (old_mode == -1)
		return;

	// first, store this frequency in the appropriate bin
	for (old_band = 0; old_band < max_bands; old_band++)
		if (band_stack[old_band].start <= old_freq && old_freq <= band_stack[old_band].stop)
			break;

	int stack = band_stack[old_band].index;
	if (stack < 0 || stack >= STACK_DEPTH)
		stack = 0;
	if (old_band < max_bands){
		// update the old band setting 
		if (stack >= 0 && stack < STACK_DEPTH){
				band_stack[old_band].freq[stack] = old_freq;
				band_stack[old_band].mode[stack] = old_mode;
		}
	}

	// if we are still in the same band, move to the next position
	if (new_band == old_band){
		stack = ++band_stack[new_band].index;
		// move the stack and wrap the band around
		if (stack >= STACK_DEPTH)
			stack = 0;
		band_stack[new_band].index = stack;
	}
	stack = band_stack[new_band].index;
	sprintf(buff, "%d", band_stack[new_band].freq[stack]);
	char resp[100];
	set_operating_freq(band_stack[new_band].freq[stack], resp);
	set_field(R1_FREQ, buff);
	set_mode(mode_name[band_stack[new_band].mode[stack]]);	
	// set_field("r1:mode", mode_name[band_stack[new_band].mode[stack]]);	

	// this fixes bug with filter settings not being applied after a band change, not sure why it's a bug - k3ng 2022-09-03
	set_field(R1_LOW, get_field(R1_LOW)->value);
	set_field(R1_HIGH, get_field(R1_HIGH)->value);

	abort_tx();
}


static void meter_calibrate(){
	// we change to 40 meters, cw
	printf("starting meter calibration\n"
	"1. Attach a power meter and a dummy load to the antenna\n"
	"2. Adjust the drive until you see 40 watts on the power meter\n"
	"3. Press the tuning knob to confirm.\n");

	set_field(R1_FREQ, "7035000");
	set_mode("CW");	
	struct field *f_bridge = get_field(BRIDGE);
	set_field(BRIDGE, "100");	
	focus_field(f_bridge);
}

void do_cmd(const char *cmd){	
	char request[1000], response[1000], buff[100];
	
	strcpy(request, cmd);			// don't mangle the original, thank you

    char left[20], right[20];
    bool haveEqual = parseEqual(cmd, left, right);
    

	if (!strcmp(request, "#close")){
		gtk_window_iconify(GTK_WINDOW(window));
	}
	else if (!strcmp(request, "#off")){
		tx_off();
		set_field(_RECORD, "OFF");
		save_user_settings(1);
		exit(0);
	}
	else if (!strcmp(request, "#tx")){	
		tx_on(TX_SOFT);
	}
	else if (!strcmp(request, "#rx")){
		tx_off();
	}
	else if (!strncmp(request, "#rit", 4))
		update_field(get_field(R1_FREQ));
	else if (!strncmp(request, "#split", 5)){
		update_field(get_field(R1_FREQ));	
		if (!strcmp(get_field(_VFO)->value, "B"))
			set_field(_VFO, "A");
	}
	else if (!strcmp(request, "#vfo=B")){
		struct field *f = get_field(R1_FREQ);
		struct field *vfo = get_field(_VFO);
		struct field *vfo_a = get_field(_VFO_A_FREQ);
		struct field *vfo_b = get_field(_VFO_B_FREQ);
		if (!strcmp(vfo->value, "B")){
			// vfo_a_freq = atoi(f->value);
			strcpy(vfo_a->value, f->value);
			// sprintf(buff, "%d", vfo_b_freq);
			set_field(R1_FREQ, vfo_b->value);
			settings_updated++;
		}
	}
	else if (!strcmp(request, "#vfo=A")){
		struct field *f = get_field(R1_FREQ);
		struct field *vfo = get_field(_VFO);
		struct field *vfo_a = get_field(_VFO_A_FREQ);
		struct field *vfo_b = get_field(_VFO_B_FREQ);
		// printf("vfo old %s, new %s\n", vfo->value, request);
		if (!strcmp(vfo->value, "A")){
			// vfo_b_freq = atoi(f->value);
			strcpy(vfo_b->value, f->value);
			// sprintf(buff, "%d", vfo_a_freq);
			set_field(R1_FREQ, vfo_a->value);
			settings_updated++;
		}
	} else if (!strcmp(request, "#step=1M"))
		// tuning step
    	tuning_step = 1000000;
	else if (!strcmp(request, "#step=100K"))
		tuning_step = 100000;
	else if (!strcmp(request, "#step=10K"))
		tuning_step = 10000;
	else if (!strcmp(request, "#step=1K"))
		tuning_step = 1000;
	else if (!strcmp(request, "#step=100H"))
		tuning_step = 100;
	else if (!strcmp(request, "#step=10H"))
		tuning_step = 10;

	// spectrum bandwidth
    if (haveEqual && !strcmp(left, "#span")) {
        right[strlen(right) - 1] = '\0';
        spectrum_span = (int) (atof(right) * 1000);
    }
    		
	// handle the band stacking
	else if (!strcmp(request, "#80m") || 
		!strcmp(request, "#40m") || 
		!strcmp(request, "#30m") || 
		!strcmp(request, "#20m") || 
		!strcmp(request, "#17m") || 
		!strcmp(request, "#15m") || 
		!strcmp(request, "#12m") || 
		!strcmp(request, "#10m")){
		change_band(request+1); //skip the hash		
	}
	else if (!strcmp(request, "#record=ON")){
		char fullpath[200];	//dangerous, find the MAX_PATH and replace 200 with it

		char *path = getenv("HOME");
		time(&record_start);
		struct tm *tmp = localtime(&record_start);
		sprintf(fullpath, "%s/sbitx/audio/%04d%02d%02d-%02d%02d-%02d.wav", path, 
			tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec); 

		char request[300], response[100];
		sprintf(request, "record=%s", fullpath);
		sdr_request(request, response);
		write_console(FONT_LOG, "Recording ");
		write_console(FONT_LOG, fullpath);
		write_console(FONT_LOG, "\n");
	}
	else if (!strcmp(request, "#record=OFF")){
		sdr_request("record", "off");
		write_console(FONT_LOG, "Recording stopped\n");
		record_start = 0;
	}
	else if (!strcmp(request, "#mfqrz") && strlen(contact_callsign) > 0)
		qrz(contact_callsign);

	//this needs to directly pass on to the sdr core
	else if(request[0] != '#'){
		//translate the frequency of operating depending upon rit, split, etc.
		if (!strncmp(request, "r1:freq", 7))
			set_operating_freq(atoi(request+8), response);
		else
			sdr_request(request, response);
	}
}

static void enter_qso(char *args){
	char contact_callsign[20];
	char rst_sent[10];
	char exchange_sent[10];
	char rst_recv[10];
	char exchange_recv[10];
	char *p, *q;
	int i = 0, len;

	q = args;	
	len = strlen(args);
	for(p = contact_callsign; *q != '|' && *q; i++)
		*p++ = *q++; 
	*p = 0; q++;

	for(p = rst_sent; *q != '|' && *q; i++)
		*p++ = *q++; 
	*p = 0; q++;

	for(p = exchange_sent; *q != '|' && *q; i++)
		*p++ = *q++; 
	*p = 0; q++;

	for(p = rst_recv; *q != '|' && *q; i++)
		*p++ = *q++; 
	*p = 0; q++;

	for(p = exchange_recv; *q != '|' && *q; i++)
		*p++ = *q++; 
	*p = 0;

	logbook_add(contact_callsign, rst_sent, exchange_sent, rst_recv, exchange_recv);
}

static void cmd_exec(char *cmd){
	int i, j;
	int mode = mode_id(get_field("r1:mode")->value);

	char args[MAX_FIELD_LENGTH];
	char exec[20];

	args[0] = 0;

	write_console(FONT_LOG, cmd);
	#ifdef N8ME
	write_console(FONT_LOG, "\n");
	#endif
	// copy the exec
	for (i = 0; *cmd > ' ' && i < sizeof(exec) - 1; i++)
		exec[i] = *cmd++;
	exec[i] = 0; 

	// skip the spaces
	while(*cmd == ' ')
		cmd++;

	j = 0;
	for (i = 0; *cmd && i < sizeof(args) - 1; i++){
		if (*cmd > ' ')
			j = i;
		args[i] = *cmd++;
	}
	args[++j] = 0;

	char response[100];
	if (!strcmp(exec, "callsign")){
		strcpy(get_field("#mycallsign")->value,args); 
		sprintf(response, "\n[Your callsign is set to %s]\n", get_field("#mycallsign")->value);
		write_console(FONT_LOG, response);
	}
	else if (!strcmp(exec, "metercal")){
		meter_calibrate();
	}
	else if (!strcmp(exec, "abort"))
		abort_tx();
	else if (!strcmp(exec, "txcal")){
		char response[10];
		sdr_request("txcal=", response);
	}
	else if (!strcmp(exec, "grid")){	
		set_field("#mygrid", args);
		sprintf(response, "\n[Your grid is set to %s]\n", get_field("#mygrid")->value);
		write_console(FONT_LOG, response);
	}
	else if (!strcmp(exec, "l")){
		interpret_log(args);
		update_log_ed();
	}
	else if (!strcmp(exec, "logbook")){
		char fullpath[200];	// dangerous, find the MAX_PATH and replace 200 with it
		char *path = getenv("HOME");
		sprintf(fullpath, "mousepad %s/sbitx/data/logbook.txt", path); 
		execute_app(fullpath);
	}
	else if (!strcmp(exec, "clear")){
		console_init();
	}
	else if(!strcmp(exec, "macro")){
		if (!strcmp(args, "list"))
			macro_list(NULL);
		else if (!macro_load(args, NULL)){
			set_ui(LAYOUT_MACROS);
			set_field("#current_macro", args);
		}
		else if (strlen(get_field("#current_macro")->value)){
			write_console(FONT_LOG, "current macro is ");
			write_console(FONT_LOG, get_field("#current_macro")->value);
			write_console(FONT_LOG, "\n");
		}
		else
			write_console(FONT_LOG, "macro file not loaded\n");
	}
	else if (!strcmp(exec, "qso"))
		enter_qso(args);
	else if (!strcmp(exec, "exchange")){
		set_field("#contest_serial", "0");
		set_field("#sent_exchange", "");

		if (strlen(args)){
			set_field("#sent_exchange", args);
			if (atoi(args))
				set_field("#contest_serial", args);
		}
		write_console(FONT_LOG, "Exchange set to [");
		write_console(FONT_LOG, get_field("#sent_exchange")->value);
		write_console(FONT_LOG, "]\n");
	}
	else if(!strcmp(exec, "freq") || !strcmp(exec, "f")){
		long freq = atol(args);
		if (freq == 0){
			write_console(FONT_LOG, "Usage: \f xxxxx (in Hz or KHz)\n");
		}
		else if (freq < 30000)
			freq *= 1000;

		if (freq > 0){
			char freq_s[20];
			sprintf(freq_s, "%ld",freq);
			set_field("r1:freq", freq_s);
		}
	} else if (!strcmp(exec, "exit")){
		tx_off();
		set_field("#record", "OFF");
		save_user_settings(1);
		exit(0);
	} else if ((!strcmp(exec, "lsb")) || (!strcmp(exec, "LSB"))){
    	set_mode("LSB");
	} else if ((!strcmp(exec, "usb")) || (!strcmp(exec, "USB"))){
    	set_mode("USB"); 
	} else if ((!strcmp(exec, "cw")) || (!strcmp(exec, "CW"))){
    	set_mode("CW"); 
	} else if ((!strcmp(exec, "cwr")) || (!strcmp(exec, "CWR"))){
    	set_mode("CWR");
  	} else if ((!strcmp(exec, "rtty")) || (!strcmp(exec, "RTTY"))){
    	set_mode("RTTY");
	} else if ((!strcmp(exec, "ft8")) || (!strcmp(exec, "FT8"))){
    	set_mode("FT8");
	} else if ((!strcmp(exec, "psk")) || (!strcmp(exec, "psk31")) || (!strcmp(exec, "PSK")) || (!strcmp(exec, "PSK31"))){
	    set_mode("PSK31");
	} else if ((!strcmp(exec, "digital")) || (!strcmp(exec, "DIGITAL"))  || (!strcmp(exec, "dig")) || (!strcmp(exec, "DIG"))){
    	set_mode("DIGITAL");
	} else if ((!strcmp(exec, "2tone")) || (!strcmp(exec, "2TONE"))){
    	set_mode("2TONE");
	} else if (!strcmp(exec, "band"))
		change_band(args); 
	else if (!strcmp(exec, "bandwidth") || !strcmp(exec, "bw")){
		set_bandwidth(atoi(args));
	} else if ((!strcmp(exec, "help")) || (!strcmp(exec, "?"))){
		extern char *help_text;
		write_console(FONT_LOG, help_text);
	} else if (!strcmp(exec, "h2")){ 
		extern char *help2_text;
		write_console(FONT_LOG, help2_text);
	} else if ((!strcmp(exec, "s")) || (!strcmp(exec, "settings"))){
		write_console(FONT_LOG, "Settings\r\n\r\n");
		char temp_string[100];
		sprintf(temp_string,"Reverse Scrolling: ");
		strcat(temp_string,get_field("reverse_scrolling")->value);
		write_console(FONT_LOG, temp_string);
		sprintf(temp_string,"\r\nTuning Acceleration: ");
		strcat(temp_string,get_field("reverse_scrolling")->value);
		write_console(FONT_LOG, temp_string);
		sprintf(temp_string,"\r\nTuning Acceleration Threshold 1: ");
		strcat(temp_string,get_field("tuning_accel_thresh1")->value);
		write_console(FONT_LOG, temp_string);
		sprintf(temp_string,"\r\nTuning Acceleration Threshold 2: ");
		strcat(temp_string,get_field("tuning_accel_thresh2")->value);
		write_console(FONT_LOG, temp_string);
		sprintf(temp_string,"\r\nMouse Pointer: ");
		strcat(temp_string,get_field("mouse_pointer")->value);
		write_console(FONT_LOG, temp_string);
    	write_console(FONT_LOG, "\r\n");
	} else if (!strcmp(exec, "qrz")){
		if(strlen(args))
			qrz(args);
		else if (strlen(contact_callsign))
			qrz(contact_callsign);
		else
			write_console(FONT_LOG, "/qrz [callsign]\n");
	} else if (!strcmp(exec, "mode") || !strcmp(exec, "m") || !strcmp(exec, "MODE"))
		set_mode(args);
	else if (!strcmp(exec, "t"))
		tx_on(TX_SOFT);
	else if (!strcmp(exec, "r"))
		tx_off();
	else if (!strcmp(exec, "telnet")){
		if (strlen(args) > 5) 
			telnet_open(args);
		else
			telnet_open(get_field("#telneturl")->value);
	} else if (!strcmp(exec, "tclose"))
		telnet_close(args);
	else if (!strcmp(exec, "tel"))
		telnet_write(args);
	else if (!strcmp(exec, "txpitch")){
		if (strlen(args)){
			int t = atoi(args);	
			if (t > 100 && t < 4000)
				set_field("#tx_pitch", args);
			else
				write_console(FONT_LOG, "cw pitch should be 100-4000");
		}
		char buff[100];
		sprintf(buff, "txpitch is set to %d Hz\n", get_cw_tx_pitch());
		write_console(FONT_LOG, buff);
	} else if (!strcmp(exec, "PITCH")){
		struct field *f = get_field_by_label(exec);
		if (f){
			set_field(f->cmd, args);
  		int mode = mode_id(get_field("r1:mode")->value);
			int bw = 4000;
			switch(mode){
				case MODE_CW:
				case MODE_CWR:
					bw = atoi(get_field("#bw_cw")->value);
					set_bandwidth(bw);
					break;
				case MODE_DIGITAL:
					bw = atoi(get_field("#bw_digital")->value);
					set_bandwidth(bw);
					break;
			}
		}
		focus_field(f);
	} else if (exec[0] == 'F' && isdigit(exec[1])){
		char buff[1000];
		printf("executing macro %s\n", exec);
		macro_exec(atoi(exec+1), buff);
		if (strlen(buff))
			set_field("#text_in", buff);
	} else {
		// see if it matches any of the fields of the UI that have FIELD_NUMBER 
		char field_name[32];
		// conver the string to upper if not already so
		for (char *p = exec; *p; p++)
			*p =  toupper(*p);
		struct field *f = get_field_by_label(exec);
		if (f){
			if (strlen(args)){
				// convert all the letters to uppercase
				for(char *p = args; *p; p++)
					*p = toupper(*p);
				if(set_field(f->cmd, args)){
					write_console(FONT_LOG, "Invalid setting:");
      			} else {
					f_focus = f_hover = f;
					// f->update_remote = 0; // prevent the spiral of updates
        			write_console(FONT_LOG, exec);
        			write_console(FONT_LOG, " ");
       	  			write_console(FONT_LOG, args);        
        			write_console(FONT_LOG, "\n");
      			}
			} else {
				// show help
				char buff[1000];

				sprintf(buff, "Usage: %s\n", f->label);
				write_console(FONT_LOG, buff);
				switch(f->value_type){
					case FIELD_NUMBER:
						sprintf(buff, "set to %d, min=%d, max=%d\n", atoi(f->value), f->min, f->max);
						break;	
					case FIELD_BUTTON:
					case FIELD_TOGGLE:
						sprintf(buff, "set to %s, This is a switch that can be toggled On/Off\n", f->value);
						break;
					case FIELD_SELECTION:
						sprintf(buff, "set to %s, permitted values are : %s\n", f->value, f->selection);
						break;
					case FIELD_TEXT:					
						sprintf(buff, "set to [%s], permissible length is from %d to %d\n", f->value, f->min, f->max);
				}
				write_console(FONT_LOG, buff);
			}	
		}
	}
	save_user_settings(0);
}

// From https://stackoverflow.com/questions/5339200/how-to-create-a-single-instance-application-in-c-or-c
static void ensure_single_instance(){
	int pid_file = open("/tmp/sbitx.pid", O_CREAT | O_RDWR, 0666);
	int rc = flock(pid_file, LOCK_EX | LOCK_NB);
	if(rc) {
		if(EWOULDBLOCK == errno){
			printf("Another instance of sbitx is already running\n");
			exit(0);
		}	
	}
}


int main( int argc, char* argv[] ) {
    char directory[PATH_MAX];

    argv_0 = argv[0];

	puts(VER_STR);


	active_layout = main_controls;

	// ensure_single_instance();

	// unlink any pending ft8 transmission
	unlink("/home/pi/sbitx/ft8tx_float.raw");
	call_wipe();

	ui_init(argc, argv);
	hw_init();
	console_init();

	q_init(&q_remote_commands, 1000); // not too many commands
	q_init(&q_tx_text, 100); // best not to have a very large q 
	setup();

	struct field *f;
	f = active_layout;

	// initialize the modulation display

	tx_mod_max = get_field("spectrum")->width;
	tx_mod_buff = malloc(sizeof(int32_t) * tx_mod_max);
	memset(tx_mod_buff, 0, sizeof(int32_t) * tx_mod_max);
	tx_mod_index = 0;
	init_waterfall();

	// set the radio to some decent defaults
	do_cmd("r1:freq=7100000");
	do_cmd("r1:mode=LSB");	
	do_cmd("#step=1000");	
    do_cmd("#span=25K");
	strcpy(vfo_a_mode, "USB");
	strcpy(vfo_b_mode, "LSB");
	set_field("#mycallsign", "VU2LCH");
	// vfo_a_freq = 14000000;
	// vfo_b_freq = 7000000;
	
	f = get_field("spectrum");
	update_field(f);
	set_volume(20000000);

	set_field("r1:freq", "7000000");
	set_field("r1:mode", "USB");
	set_field("tx_gain", "24");
	set_field("tx_power", "40");
	set_field("r1:gain", "41");
	set_field("r1:volume", "85");

	char *path = getenv("HOME");
	strcpy(directory, path);
	strcat(directory, "/sbitx/data/user_settings.ini");
    if (ini_parse(directory, user_settings_handler, NULL)<0){
		printf("Unable to load ~/sbitx/data/user_settings.ini\n"
			"Loading default.ini instead\n");
		strcpy(directory, path);
		strcat(directory, "/sbitx/data/default_settings.ini");
		ini_parse(directory, user_settings_handler, NULL);
    }

	if (strlen(get_field("#current_macro")->value))
		macro_load(get_field("#current_macro")->value, NULL);
	char buff[1000];

	// now set the frequency of operation and more to vfo_a
	// sprintf(buff, "%d", vfo_a_freq);
	set_field("r1:freq", get_field("#vfo_a_freq")->value);

	console_init();
	write_console(FONT_LOG, VER_STR);
	write_console(FONT_LOG, "\r\nEnter \\help for help\r\n");

	if (strcmp(get_field("#mycallsign")->value, "NOBODY")){
		sprintf(buff, "\nWelcome %s your grid is %s\n", 
		get_field("#mycallsign")->value, get_field("#mygrid")->value);
		write_console(FONT_LOG, buff);
	} else 
		write_console(FONT_LOG, "Set your with '\\callsign [yourcallsign]'\n"
			"Set your 6 letter grid with '\\grid [yourgrid]\n");

	set_field("#text_in", "");

	// you don't want to save the recently loaded settings
	settings_updated = 0;
	hamlib_start();
	remote_start();

	gtk_main();
  
	return 0;
}
