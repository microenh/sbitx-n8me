#pragma once

#include <cairo.h>
#include <stdint.h>

#include "fields.h"

#define MOD_MAX 800
// enum {MOD_MAX = 800};

extern int spectrum_span;

void init_waterfall();
void sdr_modulation_update(int32_t *samples, int count, double scale_up);
void draw_spectrum(struct field *f_spectrum, cairo_t *gfx);
void draw_waterfall(struct field *f, cairo_t *gfx);
// void init_spectrum(void);
