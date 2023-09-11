#include <complex.h>
#include <gtk/gtk.h>
#include <math.h>

#include "command_str.h"
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

	// f = get_field(SPECTRUM);
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
	cairo_set_source_rgb(gfx, palette[SPECTRUM_PLOT][0], palette[SPECTRUM_PLOT][1], palette[SPECTRUM_PLOT][2]);
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
static int *wf;
static GdkPixbuf *waterfall_pixbuf = NULL;
static guint8 *waterfall_map;

void init_waterfall(){
	struct field *f = get_field(WATERFALL);

	// this will store the db values of waterfall
	wf = malloc((MAX_BINS/2) * f->height * sizeof(int));
	// this will store the bitmap pixels, 3 bytes per pixel
	waterfall_map = malloc(f->width * f->height * 3);
	if (!wf || !waterfall_map){
		puts("*Error: malloc failed on waterfall buffers");
		exit(0);
	}

    #if 0
    // colors initial waterfall area
    for (int i = 0; i < f->width; i++)
		for (int j = 0; j < f->height; j++){
			int row = j * f->width * 3;
			int	index = row + i * 3;
			waterfall_map[index++] = 0;
			waterfall_map[index++] = i % 256;
			waterfall_map[index++] = j % 256;
	    }
    #endif

	// if (waterfall_pixbuf)
	//     g_object_ref(waterfall_pixbuf);

    waterfall_pixbuf = gdk_pixbuf_new_from_data(waterfall_map,
        // format,          alpha?, bit,  width,    height,    rowstride,  destryfn, data
        GDK_COLORSPACE_RGB, FALSE,  8,    f->width, f->height, f->width*3, NULL,     NULL);


	// printf("Initialized the waterfall\n");
	// printf("%ld return from pixbuff", (int)waterfall_pixbuf);	
}


void draw_waterfall(struct field *f, cairo_t *gfx){

	memmove(waterfall_map + f->width * 3, waterfall_map, f->width * (f->height - 1) * 3);

	for (int i = 0, index = 0; i < f->width; i++){
		int v = wf[i] * 2;
		if (v > 100)		// we limit ourselves to 100 db range
			v = 100;

		if (v < 20){								// r=0, g=0, increase blue
			waterfall_map[index++] = 0;
			waterfall_map[index++] = 0;
			waterfall_map[index++] = v * 12; 
		} else if (v < 40){							// r=0, increase g, blue is max
			waterfall_map[index++] = 0;
			waterfall_map[index++] = (v - 20) * 12;
			waterfall_map[index++] = 255; 
		} else if (v < 60){							// r=0, g is max, decrease b
			waterfall_map[index++] = 0;
			waterfall_map[index++] = 255; 
			waterfall_map[index++] = (60-v)*12; 
		} else if (v < 80){						 	// increase r, g is max, b=0
			waterfall_map[index++] = (v-60) * 12;
			waterfall_map[index++] = 255;
			waterfall_map[index++] = 0; 
		} else {                                    // r is max, decrease g, b=0
			waterfall_map[index++] = 255;
			waterfall_map[index++] = (100-v) * 12;
			waterfall_map[index++] = 0; 
		}
	}

	gdk_cairo_set_source_pixbuf(gfx, waterfall_pixbuf, f->x, f->y);		
	cairo_paint(gfx);
	cairo_fill(gfx);
}

static int grid_ofs = 0;

static void draw_spectrum_grid(struct field *f_spectrum, cairo_t *gfx){
	int sub_division, grid_height;
	struct field *f = f_spectrum;

	sub_division = f->width / 10;
	grid_height = f->height - (font_table[FONT_SMALL].height * 4 / 3); 

	cairo_set_line_width(gfx, 1);
	cairo_set_source_rgb(gfx, palette[SPECTRUM_GRID][0], 
		palette[SPECTRUM_GRID][1], palette[SPECTRUM_GRID][2]);

	// draw the horizontal grid
	int i;
	for (i =  0; i <= grid_height; i += grid_height / 10){
		cairo_move_to(gfx, f->x, f->y + i);
		cairo_line_to(gfx, f->x + f->width, f->y + i); 
	}

	// draw the vertical grid
	for (i = grid_ofs; i <= f->width; i += f->width / 10){
        if (i >= 0) {
		    cairo_move_to(gfx, f->x + i, f->y);
		    cairo_line_to(gfx, f->x + i, f->y + grid_height); 
        }
	}
	cairo_stroke(gfx);
}


static int grid_height = 0, starting_bin, ending_bin, pitch_col, filter_start, filter_width, f_start, freq_div, freq_ofs;
static float x_step;

// void init_spectrum(void) {
//     grid_height = 0;
// }

static int old_bw_low, old_bw_high, old_pitch, old_freq;
static char old_mode[10];
static float old_span;

static void draw_spectrum_init(struct field *f_spectrum, cairo_t *gfx){
	int y, sub_division, bw_high, bw_low, pitch, freq;
	float span;
    static const double BIN_PER_HZ = (double) MAX_BINS / RX_SAMPLE_RATE; // 1.0 / 46.875; // reciprocal of Hz/Bin


	pitch = atoi(get_field(RX_PITCH)->value);
	struct field *mode_f = get_field(R1_MODE);
	freq = atoi(get_field(R1_FREQ)->value);

	span = atof(get_field(_SPAN)->value);
	bw_high = atoi(get_field(R1_HIGH)->value);
	bw_low = atoi(get_field(R1_LOW)->value);


    if ((old_span == span)
        && (old_bw_low == bw_low)
        && (old_bw_high == bw_high)
        && (old_pitch == pitch)
        && (old_freq == freq)
        && !strcmp(old_mode, mode_f->value)) 
        return;

    old_span = span;
    old_bw_low = bw_low;
    old_bw_high = bw_high;
    old_pitch = pitch;
    old_freq = freq;
    strcpy(old_mode, mode_f->value);

	// the step is in khz, we multiply by 1000 and div 10(divisions) = 100 
	freq_div = span * 100;  

    int display_ofs = 0;
    if (span < 10.0) {
        // display_ofs = span * 400;
    }

    int fc_bin = (int) (display_ofs * BIN_PER_HZ);
    float pixels_per_hz = f_spectrum->width / (span * 1000);
    int filter_ofs = display_ofs * pixels_per_hz;

	grid_height = f_spectrum->height - ((font_table[FONT_SMALL].height * 4) / 3);
	sub_division = f_spectrum->width / 10;

    if (span <= 20 && 0) {
        grid_ofs = -(freq % freq_div) * pixels_per_hz;
        freq_ofs = -(freq % (2 * freq_div)) * pixels_per_hz;
        f_start = freq - (freq % (2 * freq_div));
    } else {
        grid_ofs = freq_ofs = 0;
        f_start = freq  - (4 * freq_div) - display_ofs; 
        // printf("f_start: %d\r\n", f_start);
    }

	// calculate the position of bandwidth strip
	if(!strcmp(mode_f->value, "CWR") || !strcmp(mode_f->value, "LSB")){
        // LSB modes
	 	filter_start = f_spectrum->x + (f_spectrum->width/2) - (bw_high * pixels_per_hz) + filter_ofs; 
		if (filter_start < f_spectrum->x){
	 	    filter_width = (bw_high - bw_low) * pixels_per_hz - (f_spectrum->x - filter_start); 
			filter_start = f_spectrum->x;
		} else {
			filter_width = (bw_high - bw_low) * pixels_per_hz; 
		}
		if (filter_width + filter_start > f_spectrum->x + f_spectrum->width)
			filter_width = f_spectrum->x + f_spectrum->width - filter_start;
		pitch_col = f_spectrum->x + (f_spectrum->width/2) - (pitch * pixels_per_hz) + filter_ofs;
	} else {
        // USB modes
        fc_bin = -fc_bin;
        filter_ofs = -filter_ofs;
        display_ofs = -display_ofs;
		filter_start = f_spectrum->x + (f_spectrum->width/2) + (bw_low * pixels_per_hz) + filter_ofs; 
		if (filter_start < f_spectrum->x)
			filter_start = f_spectrum->x;
		filter_width = (bw_high - bw_low) * pixels_per_hz; 
		if (filter_width + filter_start > f_spectrum->x + f_spectrum->width)
			filter_width = f_spectrum->x + f_spectrum->width - filter_start;
		pitch_col = f_spectrum->x + (f_spectrum->width/2) + pitch * pixels_per_hz + filter_ofs;
	}
    if(!strcmp(mode_f->value, "USB") || !strcmp(mode_f->value, "LSB") || !strcmp(mode_f->value, "FT8")){ // for LSB, USB and FT8 draw pitch line at center (carrier freq)
        pitch_col = f_spectrum->x + (f_spectrum->width/2) + filter_ofs;
    }


	// we only plot the second half of the bins (on the lower sideband)

	int n_bins = (int)((1.0 * spectrum_span) * BIN_PER_HZ);
	// the center frequency is at the center of the lower sideband,
	// i.e, three-fourth way up the bins.
	// int starting_bin = (3 *MAX_BINS) / 4 - n_bins / 2;
	starting_bin = (3 *MAX_BINS) / 4 - n_bins / 2 + fc_bin;
	ending_bin = starting_bin + n_bins; 

	x_step = (1.0 * f_spectrum->width ) / n_bins;
}

void draw_spectrum(struct field *f_spectrum, cairo_t *gfx){

	if (in_tx){
		draw_modulation(f_spectrum, gfx);
		return;
	}

    // should only need to be called when mode, span, filter_low, filter_high or pitch changes
    draw_spectrum_init(f_spectrum, gfx);

	// clear the spectrum	
	fill_rect(gfx, f_spectrum->x, f_spectrum->y, f_spectrum->width, f_spectrum->height, SPECTRUM_BACKGROUND);


	// draw the frequency readout at the bottom
    int inc = f_spectrum->width / 5;
    int tgt = f_spectrum->x + f_spectrum->width;
    char freq_text[20];
	cairo_set_source_rgb(gfx, palette[COLOR_TEXT_MUTED][0], palette[COLOR_TEXT_MUTED][1], palette[COLOR_TEXT_MUTED][2]);
	for (int i = f_spectrum->width/10 + freq_ofs + f_spectrum->x, f_work = f_start; i < tgt; i += inc, f_work += 2 * freq_div) {
        freq_with_separators(freq_text, f_work);
        int off = measure_text(gfx, freq_text, FONT_SMALL) / 2;
        if (i >= off)
            draw_text(gfx, i - off, f_spectrum->y + grid_height, freq_text, FONT_SMALL);
	}


    // background for the selected bandwidth
	fill_rect(gfx, filter_start, f_spectrum->y, filter_width, grid_height, SPECTRUM_BANDWIDTH);  

	draw_spectrum_grid(f_spectrum, gfx);

	// we only plot the second half of the bins (on the lower sideband)


	// start the plot
	cairo_set_source_rgb(gfx, palette[SPECTRUM_PLOT][0], palette[SPECTRUM_PLOT][1], palette[SPECTRUM_PLOT][2]);
	cairo_move_to(gfx, f_spectrum->x + f_spectrum->width, f_spectrum->y + grid_height);

	float x = 0;
	int j = 0;
	for (int i = starting_bin; i <= ending_bin; i++){
		int y;

		// the center fft bin is at zero, from MAX_BINS/2 onwards,
		// the bins are at lowest frequency (negative frequency)
		// y axis is the power in db of each bin, scaled to 80 db
		// y = ((spectrum_plot[i] + waterfall_offset) * f_spectrum->height) / 80; 
		y = (((10 * log10f(cnrmf(fft_spectrum[i]))) + waterfall_offset) * f_spectrum->height) / 80; 
		// y = (((10 * log10f(cnrmf(fft_out[i]))) + waterfall_offset) * f_spectrum->height) / 80; 
		// limit y inside the spectrum display box
		if (y <  0)
			y = 0;
		if (y > f_spectrum->height)
			y = f_spectrum->height - 1;
		// the plot should be increase upwards
		cairo_line_to(gfx, f_spectrum->x + f_spectrum->width - (int)x, f_spectrum->y + grid_height - y);

		// fill the waterfall
		for (int k = 0; k <= 1 + (int)x_step; k++)
			wf[k + f_spectrum->width - (int)x] = (y * 100)/grid_height;
		x += x_step;
	}
	cairo_stroke(gfx);
 
    // draw carrier (USB, LSB, FT8) or rx pitch
    cairo_set_source_rgb(gfx, palette[SPECTRUM_PITCH][0],palette[SPECTRUM_PITCH][1], palette[SPECTRUM_PITCH][2]);
    cairo_move_to(gfx, pitch_col, f_spectrum->y);
    cairo_line_to(gfx, pitch_col, f_spectrum->y + grid_height); 
    cairo_stroke(gfx);

	// draw the needle
    #if 0
	for (struct rx *r = rx_list; r; r = r->next){
		int needle_x  = (f_spectrum->width*(MAX_BINS/2 - r->tuned_bin))/(MAX_BINS/2) - filter_ofs;
		fill_rect(gfx, f_spectrum->x + needle_x, f_spectrum->y, 1, grid_height,  SPECTRUM_NEEDLE);
	}
    #endif

    // [mee] 8/23/23 - already called from do_waterfall
    // draw_waterfall(get_field(WATERFALL), gfx);
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

#if 0
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
#endif