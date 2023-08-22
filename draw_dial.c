#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "draw_dial.h"
#include "drawing.h"
#include "sdr.h"

static char* freq_with_separators(char* freq_str){

	int freq = atoi(freq_str);
	int f_mhz, f_khz, f_hz;
	char temp_string[11];
	static char return_string[11];

	f_mhz = freq / 1000000;
	f_khz = (freq - (f_mhz*1000000)) / 1000;
	f_hz = freq - (f_mhz*1000000) - (f_khz*1000);

	sprintf(temp_string,"%d",f_mhz);
	strcpy(return_string,temp_string);
	strcat(return_string,".");
	if (f_khz < 100){
    	strcat(return_string,"0");
	}
	if (f_khz < 10){
    	strcat(return_string,"0");
	}
	sprintf(temp_string,"%d",f_khz);
	strcat(return_string,temp_string);
	strcat(return_string,".");
	if (f_hz < 100){
	    strcat(return_string,"0");
	}
	if (f_hz < 10){
    	strcat(return_string,"0");
	}
	sprintf(temp_string,"%d",f_hz);
	strcat(return_string,temp_string);
	return return_string;
}



void draw_dial(struct field *f, cairo_t *gfx){
    const int LABEL_FONT = FONT_SMALL;
    const int TOP_FONT = FONT_LARGE_VALUE;
    const int BOTTOM_FONT = FONT_LARGE_VALUE;

    const int LABEL_Y_OFFSET = 5;
    const int TOP_VFO_Y_OFFSET = 16;
    const int BOT_VFO_Y_OFFSET = 35;
    const int VFO_X_OFFSET = 5;

    const char *TX_BUF = "TX: %s";
    const char *RX_BUF = "RX: %s";
    const char *A_BUF = "A:  %s";
    const char *B_BUF = "B:  %s";

    #if 0
	struct field *rit = get_field("#rit");
	struct field *split = get_field("#split");
	struct field *vfo = get_field("#vfo");
	struct field *vfo_a = get_field("#vfo_a_freq");
	struct field *vfo_b = get_field("#vfo_b_freq");
	struct field *rit_delta = get_field("#rit_delta");
    #endif

	static struct field *rit;
	static struct field *split;
	static struct field *vfo;
	static struct field *vfo_a;
	static struct field *vfo_b;
	static struct field *rit_delta;

	char buff[20];
	char temp_str[20];

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
        top_x = f->x + 5;
        bottom_x = top_x;

        rit = get_field("#rit");
        split = get_field("#split");
        vfo = get_field("#vfo");
        vfo_a = get_field("#vfo_a_freq");
        vfo_b = get_field("#vfo_b_freq");
        rit_delta = get_field("#rit_delta");
    }

	fill_rect(gfx, f->x+1, f->y+1, f->width-2, f->height-2, COLOR_BACKGROUND);
    draw_text(gfx, label_x, label_y, f->label, LABEL_FONT);

	// update the vfos
	if (vfo->value[0] == 'A')
		strcpy(vfo_a->value, f->value);
	else
		strcpy(vfo_b->value, f->value);


	int width, offset, freq_top, freq_bottom, offset_y = LABEL_Y_OFFSET;
    char *top_buf, *bottom_buf;
	
	if (!strcmp(rit->value, "ON")){
    	if (!in_tx){
    		sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
    		sprintf(buff, RX_BUF, freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    		sprintf(buff, TX_BUF, freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    	} else {
    		sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
    		sprintf(buff, RX_BUF, freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    		sprintf(buff, TX_BUF, freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    	}
	} else if (!strcmp(split->value, "ON")){
		if (!in_tx){
			sprintf(buff, RX_BUF, freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, TX_BUF, freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		} else {
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, TX_BUF, freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, RX_BUF, atoi(f->value) + atoi(rit_delta->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		}
	} else if (!strcmp(vfo->value, "B")){
		if (!in_tx){
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, B_BUF, freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, A_BUF, freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		} else {
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, B_BUF, freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, TX_BUF, freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		}
	} else {
		if (!in_tx){
			strcpy(temp_str, vfo_a->value);
    		// sprintf(temp_str, "%d", vfo_a_freq);
    		sprintf(buff, A_BUF, freq_with_separators(temp_str));
    		draw_text(gfx, top_x, top_y, buff, TOP_FONT);
    		sprintf(buff, B_BUF, freq_with_separators(f->value));
    		draw_text(gfx, bottom_x, bottom_y, buff, BOTTOM_FONT);
    	} else {
			strcpy(temp_str, vfo_a->value);
    		// sprintf(temp_str, "%d", vfo_a_freq);
    		sprintf(buff, A_BUF, freq_with_separators(temp_str));
    		draw_text(gfx, top_x, top_y, buff, TOP_FONT);
    		sprintf(buff, TX_BUF, freq_with_separators(f->value));
    		draw_text(gfx, bottom_x, bottom_y, buff, BOTTOM_FONT);
    	}
	}
}
