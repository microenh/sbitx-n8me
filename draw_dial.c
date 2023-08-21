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


const int FREQ_LABEL_Y_OFFSET = 5;
const int TOP_VFO_Y_OFFSET = 16;
const int BOT_VFO_Y_OFFSET = 35;
const int VFO_X_OFFSET = 5;

void draw_dial(struct field *f, cairo_t *gfx){
	struct font_style *s = font_table + 0;
	struct field *rit = get_field("#rit");
	struct field *split = get_field("#split");
	struct field *vfo = get_field("#vfo");
	struct field *vfo_a = get_field("#vfo_a_freq");
	struct field *vfo_b = get_field("#vfo_b_freq");
	struct field *rit_delta = get_field("#rit_delta");
	char buff[20];

	char temp_str[20];

	fill_rect(gfx, f->x+1, f->y+1, f->width-2,f->height-2, COLOR_BACKGROUND);

	//update the vfos
	if (vfo->value[0] == 'A')
		strcpy(vfo_a->value, f->value);
	else
		strcpy(vfo_b->value, f->value);

	int width, offset;	
	
	width = measure_text(gfx, f->label, FONT_SMALL);
	offset = f->width/2 - width/2;
	draw_text(gfx, f->x + offset, f->y+FREQ_LABEL_Y_OFFSET ,  f->label, FONT_SMALL);
	width = measure_text(gfx, f->value, f->font_index);
	offset = f->width/2 - width/2;
	if (!strcmp(rit->value, "ON")){
    	if (!in_tx){
    		sprintf(buff, "TX:%s", freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    		sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
    		sprintf(buff, "RX:%s", freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    	} else {
    		sprintf(buff, "TX:%s", freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    		sprintf(temp_str, "%d", (atoi(f->value) + atoi(rit_delta->value)));
    		sprintf(buff, "RX:%s", freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    	}
	} else if (!strcmp(split->value, "ON")){
		if (!in_tx){
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, "TX:%s", freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
			sprintf(buff, "RX:%s", freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
		} else {
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, "TX:%s", freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, "RX:%d", atoi(f->value) + atoi(rit_delta->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		}
	} else if (!strcmp(vfo->value, "B")){
		if (!in_tx){
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, "B:%s", freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, "A:%s", freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		} else {
			// sprintf(temp_str, "%d", vfo_b_freq);
			strcpy(temp_str, vfo_b->value);
			sprintf(buff, "B:%s", freq_with_separators(temp_str));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
			sprintf(buff, "TX:%s", freq_with_separators(f->value));
			draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
		}
	} else {
		if (!in_tx){
			strcpy(temp_str, vfo_a->value);
    		// sprintf(temp_str, "%d", vfo_a_freq);
    		sprintf(buff, "A:%s", freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    		sprintf(buff, "B:%s", freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    	} else {
			strcpy(temp_str, vfo_a->value);
    		// sprintf(temp_str, "%d", vfo_a_freq);
    		sprintf(buff, "A:%s", freq_with_separators(temp_str));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+TOP_VFO_Y_OFFSET , buff , FONT_LARGE_VALUE);
    		sprintf(buff, "TX:%s", freq_with_separators(f->value));
    		draw_text(gfx, f->x+VFO_X_OFFSET , f->y+BOT_VFO_Y_OFFSET , buff , FONT_SMALL);
    	}
	}
}
