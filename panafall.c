#include <gtk/gtk.h>

#include "drawing.h"
#include "fonts.h"
#include "panafall.h"
#include "sdr.h"

// mod disiplay holds the tx modulation time domain envelope
// even values are the maximum and the even values are minimum

int spectrum_span = 48000;

static int mod_display[MOD_MAX];
static int mod_display_index = 0;

void sdr_modulation_update(int32_t *samples, int count, double scale_up){
	double min=0, max=0;

	for (int i = 0; i < count; i++){
		if (i % 48 == 0 && i > 0){
			if (mod_display_index >= MOD_MAX)
				mod_display_index = 0;
			mod_display[mod_display_index++] = (min / 40000000.0) / scale_up;
			mod_display[mod_display_index++] = (max / 40000000.0) / scale_up;
			min = 0x7fffffff;
			max = -0x7fffffff;
		}
		if (*samples < min)
			min = *samples;
		if (*samples > max)
			max = *samples;
		samples++;
	}
}


static void draw_modulation(struct field *f, cairo_t *gfx){

	int y, sub_division, i, grid_height;
	long	freq, freq_div;
	char	freq_text[20];

	// f = get_field("spectrum");
	sub_division = f->width / 10;
	grid_height = f->height - 10;

	// clear the spectrum	
	fill_rect(gfx, f->x,f->y, f->width, f->height, SPECTRUM_BACKGROUND);
	cairo_stroke(gfx);
	cairo_set_line_width(gfx, 1);
	cairo_set_source_rgb(gfx, palette[SPECTRUM_GRID][0], palette[SPECTRUM_GRID][1], palette[SPECTRUM_GRID][2]);

	// draw the horizontal grid
	for (i =  0; i <= grid_height; i += grid_height/10){
		cairo_move_to(gfx, f->x, f->y + i);
		cairo_line_to(gfx, f->x + f->width, f->y + i); 
	}

	// draw the vertical grid
	for (i = 0; i <= f->width; i += f->width/10){
		cairo_move_to(gfx, f->x + i, f->y);
		cairo_line_to(gfx, f->x + i, f->y + grid_height); 
	}
	cairo_stroke(gfx);

	// start the plot
	cairo_set_source_rgb(gfx, palette[SPECTRUM_PLOT][0], 
		palette[SPECTRUM_PLOT][1], palette[SPECTRUM_PLOT][2]);
	cairo_move_to(gfx, f->x + f->width, f->y + grid_height);


	int n_env_samples = sizeof(mod_display)/sizeof(int32_t);		
	int h_center = f->y + grid_height / 2;
	for (i = 0; i < f->width; i++){
		int index = (i * n_env_samples)/f->width;
		int min = mod_display[index++];
		int max = mod_display[index++]; 
		cairo_move_to(gfx, f->x + i ,  min + h_center);
		cairo_line_to(gfx, f->x + i,   max + h_center + 1);
	}
	cairo_stroke(gfx);
}

static int waterfall_offset = 30;
static int  *wf;
static GdkPixbuf *waterfall_pixbuf = NULL;
static guint8 *waterfall_map;

void init_waterfall(){
	struct field *f = get_field("waterfall");

	// this will store the db values of waterfall
	wf = malloc((MAX_BINS/2) * f->height * sizeof(int));
	if (!wf){
		puts("*Error: malloc failed on waterfall buffer");
		exit(0);
	}
	memset(wf, 0, (MAX_BINS/2) * f->height * sizeof(int));

	// this will store the bitmap pixles, 3 bytes per pixel
	waterfall_map = malloc(f->width * f->height * 3);
	for (int i = 0; i < f->width; i++)
		for (int j = 0; j < f->height; j++){
			int row = j * f->width * 3;
			int	index = row + i * 3;
			waterfall_map[index++] = 0;
			waterfall_map[index++] = i % 256;
			waterfall_map[index++] = j % 256; 
	}
	if (waterfall_pixbuf)
		g_object_ref(waterfall_pixbuf);
	waterfall_pixbuf = gdk_pixbuf_new_from_data(waterfall_map,
		GDK_COLORSPACE_RGB, FALSE, 8, f->width, f->height, f->width*3, NULL,NULL);
		// format,         alpha?, bit,  widht,    height, rowstride, destryfn, data

	printf("Initialized the waterfall\n");
	// printf("%ld return from pixbuff", (int)waterfall_pixbuf);	
}


void draw_waterfall(struct field *f, cairo_t *gfx){

	memmove(waterfall_map + f->width * 3, waterfall_map, 
		f->width * (f->height - 1) * 3);

	int index = 0;
	
	for (int i = 0; i < f->width; i++){
		int v = wf[i] * 2;
		if (v > 100)		// we limit ourselves to 100 db range
			v = 100;

		if (v < 20){								// r=0, g=0, increase blue
			waterfall_map[index++] = 0;
			waterfall_map[index++] = 0;
			waterfall_map[index++] = v * 12; 
		}
		else if (v < 40){							// r=0, increase g, blue is max
			waterfall_map[index++] = 0;
			waterfall_map[index++] = (v - 20) * 12;
			waterfall_map[index++] = 255; 
		}
		else if (v < 60){							// r=0, g is max, decrease b
			waterfall_map[index++] = 0;
			waterfall_map[index++] = 255; 
			waterfall_map[index++] = (60-v)*12; 
		}
		else if (v < 80){						 	// increase r, g is max, b=0
			waterfall_map[index++] = (v-60) * 12;
			waterfall_map[index++] = 255;
			waterfall_map[index++] = 0; 
		}else {										// r is max, decrease g, b=0
			waterfall_map[index++] = 255;
			waterfall_map[index++] = (100-v) * 12;
			waterfall_map[index++] = 0; 
		}
	}

	gdk_cairo_set_source_pixbuf(gfx, waterfall_pixbuf, f->x, f->y);		
	cairo_paint(gfx);
	cairo_fill(gfx);
}

static void draw_spectrum_grid(struct field *f_spectrum, cairo_t *gfx){
	int sub_division, grid_height;
	struct field *f = f_spectrum;

	sub_division = f->width / 10;
	grid_height = f->height - (font_table[FONT_SMALL].height * 4 / 3); 

	cairo_set_line_width(gfx, 1);
	cairo_set_source_rgb(gfx, palette[SPECTRUM_GRID][0], 
		palette[SPECTRUM_GRID][1], palette[SPECTRUM_GRID][2]);


	cairo_set_line_width(gfx, 1);
	cairo_set_source_rgb(gfx, palette[SPECTRUM_GRID][0], 
		palette[SPECTRUM_GRID][1], palette[SPECTRUM_GRID][2]);

	// draw the horizontal grid
	int i;
	for (i =  0; i <= grid_height; i += grid_height/10){
		cairo_move_to(gfx, f->x, f->y + i);
		cairo_line_to(gfx, f->x + f->width, f->y + i); 
	}

	// draw the vertical grid
	for (i = 0; i <= f->width; i += f->width/10){
		cairo_move_to(gfx, f->x + i, f->y);
		cairo_line_to(gfx, f->x + i, f->y + grid_height); 
	}
	cairo_stroke(gfx);
}

void draw_spectrum(struct field *f_spectrum, cairo_t *gfx){
	int y, sub_division, i, grid_height, bw_high, bw_low, pitch;
	float span;
	struct field *f;
	long	freq, freq_div;
	char	freq_text[20];

	if (in_tx){
		draw_modulation(f_spectrum, gfx);
		return;
	}

	pitch = atoi(get_field("rx_pitch")->value);
	struct field *mode_f = get_field("r1:mode");
	freq = atol(get_field("r1:freq")->value);

	span = atof(get_field("#span")->value);
	bw_high = atoi(get_field("r1:high")->value);
	bw_low = atoi(get_field("r1:low")->value);
	grid_height = f_spectrum->height - ((font_table[FONT_SMALL].height * 4) /3);
	sub_division = f_spectrum->width / 10;

	// the step is in khz, we multiply by 1000 and div 10(divisions) = 100 
	freq_div = span * 100;  

	// calculate the position of bandwidth strip
	int filter_start, filter_width;

	if(!strcmp(mode_f->value, "CWR") || !strcmp(mode_f->value, "LSB")){
	 	filter_start = f_spectrum->x + (f_spectrum->width/2) - 
			((f_spectrum->width * bw_high)/(span * 1000)); 
		if (filter_start < f_spectrum->x){
	 	  filter_width = ((f_spectrum->width * (bw_high -bw_low))/(span * 1000)) - (f_spectrum->x - filter_start); 
			filter_start = f_spectrum->x;
		} else {
			filter_width = (f_spectrum->width * (bw_high -bw_low))/(span * 1000); 
		}
		if (filter_width + filter_start > f_spectrum->x + f_spectrum->width)
			filter_width = f_spectrum->x + f_spectrum->width - filter_start;
		pitch = f_spectrum->x + (f_spectrum->width/2) -
			((f_spectrum->width * pitch)/(span * 1000));
	} else {
		filter_start = f_spectrum->x + (f_spectrum->width/2) + 
			((f_spectrum->width * bw_low)/(span * 1000)); 
		if (filter_start < f_spectrum->x)
			filter_start = f_spectrum->x;
		filter_width = (f_spectrum->width * (bw_high-bw_low))/(span * 1000); 
		if (filter_width + filter_start > f_spectrum->x + f_spectrum->width)
			filter_width = f_spectrum->x + f_spectrum->width - filter_start;
		pitch = f_spectrum->x + (f_spectrum->width/2) + 
			((f_spectrum->width * pitch)/(span * 1000));
	}
	// clear the spectrum	
	f = f_spectrum;
	fill_rect(gfx, f->x,f->y, f->width, f->height, SPECTRUM_BACKGROUND);
	cairo_stroke(gfx);
	fill_rect(gfx, filter_start,f->y,filter_width,grid_height,SPECTRUM_BANDWIDTH);  
	cairo_stroke(gfx);

	draw_spectrum_grid(f_spectrum, gfx);
	f = f_spectrum;

	// draw the frequency readout at the bottom
	cairo_set_source_rgb(gfx, palette[COLOR_TEXT_MUTED][0], 
		palette[COLOR_TEXT_MUTED][1], palette[COLOR_TEXT_MUTED][2]);
	long f_start = freq - (4 * freq_div); 
	for (i = f->width/10; i < f->width; i += f->width/10){
		if ((span == 25) || (span == 10)){
			sprintf(freq_text, "%ld", f_start/1000);
		} else {
			float f_start_temp = (((float)f_start/1000000.0) - ((int)(f_start/1000000))) *1000;
			sprintf(freq_text, "%5.1f", f_start_temp);
		}
		int off = measure_text(gfx, freq_text, FONT_SMALL)/2;
		draw_text(gfx, f->x + i - off , f->y+grid_height , freq_text, FONT_SMALL);
		f_start += freq_div;
	}

	// we only plot the second half of the bins (on the lower sideband
	int last_y = 100;

	int n_bins = (int)((1.0 * spectrum_span) / 46.875);
	// the center frequency is at the center of the lower sideband,
	// i.e, three-fourth way up the bins.
	int starting_bin = (3 *MAX_BINS)/4 - n_bins/2;
	int ending_bin = starting_bin + n_bins; 

	float x_step = (1.0 * f->width )/n_bins;

	// start the plot
	cairo_set_source_rgb(gfx, palette[SPECTRUM_PLOT][0], 
		palette[SPECTRUM_PLOT][1], palette[SPECTRUM_PLOT][2]);
	cairo_move_to(gfx, f->x + f->width, f->y + grid_height);

	float x = 0;
	int j = 0;
	for (i = starting_bin; i <= ending_bin; i++){
		int y;

		// the center fft bin is at zero, from MAX_BINS/2 onwards,
		// the bins are at lowest frequency (-ve frequency)
		// y axis is the power  in db of each bin, scaled to 80 db
		y = ((spectrum_plot[i] + waterfall_offset) * f->height)/80; 
		// limit y inside the spectrum display box
		if ( y <  0)
			y = 0;
		if (y > f->height)
			y = f->height - 1;
		// the plot should be increase upwards
		cairo_line_to(gfx, f->x + f->width - (int)x, f->y + grid_height - y);

		// fill the waterfall
		for (int k = 0; k <= 1 + (int)x_step; k++)
			wf[k + f->width - (int)x] = (y * 100)/grid_height;
		x += x_step;
	}
	cairo_stroke(gfx);
 
	if (pitch >= f_spectrum->x){
    	cairo_set_source_rgb(gfx, palette[SPECTRUM_PITCH][0],palette[SPECTRUM_PITCH][1], palette[SPECTRUM_PITCH][2]);
    	if(!strcmp(mode_f->value, "USB") || !strcmp(mode_f->value, "LSB")){ // for LSB and USB draw pitch line at center
	    	cairo_move_to(gfx, f->x + (f->width/2), f->y);
	    	cairo_line_to(gfx, f->x + (f->width/2), f->y + grid_height); 
    	} else {
	    	cairo_move_to(gfx, pitch, f->y);
	   	 	cairo_line_to(gfx, pitch, f->y + grid_height); 
    	}
   		cairo_stroke(gfx);
	}

	// draw the needle
	for (struct rx *r = rx_list; r; r = r->next){
		int needle_x  = (f->width*(MAX_BINS/2 - r->tuned_bin))/(MAX_BINS/2);
		fill_rect(gfx, f->x + needle_x, f->y, 1, grid_height,  SPECTRUM_NEEDLE);
	}

	draw_waterfall(get_field("waterfall"), gfx);
}

#if 0
static int waterfall_fn(struct field *f, cairo_t *gfx, int event, int a, int b){
	if(f->fn(f, gfx, FIELD_DRAW, -1, -1, 0))
		switch(FIELD_DRAW){
			case FIELD_DRAW:
				draw_waterfall(f, gfx);
				break;
		}
}
#endif

void web_get_spectrum(char *buff){

	int n_bins = (int)((1.0 * spectrum_span) / 46.875);
	// the center frequency is at the center of the lower sideband,
	// i.e, three-fourth way up the bins.
	int starting_bin = (3 *MAX_BINS)/4 - n_bins/2;
	int ending_bin = starting_bin + n_bins;

	int j = 3;
	if (in_tx){
		strcpy(buff, "TX ");
    	for (int i = 0; i < MOD_MAX; i++){
			int y = (2 * mod_display[i]) + 32;
			if (y > 127)
				buff[j++] = 127;
			else if(y > 0 && y <= 95)
				buff[j++] = y + 32;
			else
			buff[j++] = ' ';
		}
	} else{
	    strcpy(buff, "RX ");
    	for (int i = starting_bin; i <= ending_bin; i++){
			int y = spectrum_plot[i] + waterfall_offset;
			if (y > 95)
				buff[j++] = 127;
			else if(y >= 0 )
				buff[j++] = y + 32;
			else
				buff[j++] = ' ';
		}
	}

	buff[j++] = 0;
	return;
}
