#pragma ONCE

enum {
    FONT_FIELD_LABEL,
    FONT_FIELD_VALUE,
    FONT_LARGE_FIELD,
    FONT_LARGE_VALUE,
    FONT_SMALL,
    FONT_LOG,
    FONT_FT8_RX,
    FONT_FT8_TX,
    FONT_SMALL_FIELD_VALUE,
    FONT_CW_RX,
    FONT_CW_TX,
    FONT_FLDIGI_RX,
    FONT_FLDIGI_TX,
    FONT_TELNET,
    FONT_FT8_QUEUED
};

// we just use a look-up table to define the fonts used
// the struct field indexes into this table
struct font_style {
	int index;
	double r, g, b;
	const char *name;
	int height;
	int weight;
	int type;
};

extern struct font_style font_table[];

extern const int FONT_COUNT;