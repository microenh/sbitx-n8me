#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_str.h"
#include "draw_dial.h"
#include "drawing.h"
#include "sdr.h"

static void freq_with_separators(char *return_string, char *freq_str){
	int freq = atoi(freq_str);
	int f_mhz = freq / 1000000;
	int f_khz = (freq % 1000000) / 1000;
	int f_hz = (freq % 1000) / 10;

	sprintf(return_string,"%d.%03d.%02d",f_mhz, f_khz, f_hz);
}


void draw_dial(struct field *f, cairo_t *gfx){
    const int LABEL_FONT = FONT_SMALL;
    const int TOP_FONT = FONT_LARGE_VALUE;
    const int BOTTOM_FONT = FONT_SMALL;

    const int X_INSET = 5;

    static char TX_LABEL[] = "TX";
    static char RX_LABEL[] = "RX";
    static char A_LABEL[] = "A";
    static char B_LABEL[] = "B";
    static char FMT[] = "%s:  %s";

	static struct field *rit;
	static struct field *split;
	static struct field *vfo;
	static struct field *vfo_a;
	static struct field *vfo_b;
	static struct field *rit_delta;

    struct field *active_vfo;
    struct field *inactive_vfo;

	char buffer[20];
	char freq_str[20];
    // char output_str[20];

    char *top_label;
    char *bottom_label;
    char *top_text;
    char *bottom_text;

    // initialize screen locations and field pointers
    static int label_x = -1, label_y, top_x, top_y, bottom_x, bottom_y;
    if (label_x == -1) {
        label_x = f->x + (f->width - measure_text(gfx, f->label, LABEL_FONT)) / 2;
        int label_h = measure_text_y(gfx, LABEL_FONT);
        int top_h = measure_text_y(gfx, TOP_FONT);
        int bottom_h = measure_text_y(gfx, BOTTOM_FONT);
        // printf("%d, %d, %d\r\n", label_h, top_h, bottom_h);
        label_y = f->y + (f->height - (label_h + top_h + bottom_h)) / 2;
        top_y = label_y + label_h;
        bottom_y = top_y + top_h;
        bottom_x = top_x = f->x + X_INSET;
    
        rit = get_field(_RIT);
        split = get_field(_SPLIT);
        vfo = get_field(_VFO);
        vfo_a = get_field(_VFO_A_FREQ);
        vfo_b = get_field(_VFO_B_FREQ);
        rit_delta = get_field(_RIT_DELTA);
    }

	if (vfo->value[0] == 'A') {
        active_vfo = vfo_a;
        inactive_vfo = vfo_b;
    } else {
        active_vfo = vfo_b;
        inactive_vfo = vfo_a;
    }
	// update the active vfo
    strcpy(active_vfo->value, f->value);
	
	if (!strcmp(rit->value, "ON")){
        sprintf(buffer, "%d", (atoi(active_vfo->value) + atoi(rit_delta->value)));
    	if (!in_tx){
            top_text = buffer;
            top_label = RX_LABEL;
            bottom_text = f->value;
            bottom_label = TX_LABEL;
    	} else {
            top_text = f->value;
            top_label = TX_LABEL;
            bottom_text = buffer;
            bottom_label = RX_LABEL;
    	}
	} else if (!strcmp(split->value, "ON")){
		if (!in_tx){
            top_text = active_vfo->value;
            top_label = RX_LABEL;
            bottom_text = inactive_vfo->value;
            bottom_label = TX_LABEL;
		} else {
            top_text = inactive_vfo->value;
            top_label = TX_LABEL;
            bottom_text = active_vfo->value;
            bottom_label = RX_LABEL;
		}
	} else if (!strcmp(vfo->value, "B")){
		if (!in_tx){
            top_text = vfo_b->value;
            top_label = B_LABEL;
            bottom_text = vfo_a->value;
            bottom_label = A_LABEL;
		} else {
            top_text = vfo_b->value;
            top_label = B_LABEL;
            bottom_text = f->value;
            bottom_label = A_LABEL;
		}
	} else {
		if (!in_tx){
            top_text = vfo_a->value;
            top_label = A_LABEL;
            bottom_text = vfo_b->value;
            bottom_label = B_LABEL;
    	} else {
            top_text = vfo_a->value;
            top_label = A_LABEL;
            bottom_text = vfo_b->value;
            bottom_label = B_LABEL;
    	}
	}
	fill_rect(gfx, f->x+1, f->y+1, f->width-2, f->height-2, COLOR_BACKGROUND);
    draw_text(gfx, label_x, label_y, f->label, LABEL_FONT);

    draw_text(gfx, top_x, top_y, top_label, TOP_FONT);
    freq_with_separators(freq_str, top_text);
    draw_text(gfx, f->x + f->width - X_INSET - measure_text(gfx, freq_str, TOP_FONT), top_y, freq_str, TOP_FONT);

    draw_text(gfx, bottom_x, bottom_y, bottom_label, BOTTOM_FONT);
    freq_with_separators(freq_str, bottom_text);
    draw_text(gfx, f->x + f->width - X_INSET - measure_text(gfx, freq_str, BOTTOM_FONT), bottom_y, freq_str, BOTTOM_FONT);
}
