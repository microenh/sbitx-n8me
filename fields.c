#include <cairo.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "command_str.h"
#include "fields.h"
#include "fonts.h"

static const char MODE_SEL[] = "USB/LSB/CW/CWR/FT8/PSK31/RTTY/DIGITAL/2TONE";
static const char AGC_SEL[] = "OFF/SLOW/MED/FAST";
static const char ON_OFF_SEL[] = "ON/OFF";
static const char RX_TX_SEL[] = "RX/TX";
static const char STATUS_SEL[] = "status";
static const char TUNE_STEP_SEL[] = "1M/100K/10K/1K/100H/10H";
static const char A_B_SEL[] = "A/B";
static const char SPECT_WIDTH_SEL[] = "22K/20K/15K/10K/6K/5K/4K/3K/2K/1K";
static const char NOTHING_SEL[] = "nothing valuable";
static const char MOUSE_SEL[] = "BLANK/LEFT/RIGHT/CROSSHAIR";
static const char KEYER_SEL[] = "KEYBOARD/IAMBIC/IAMBICB/STRAIGHT";

// the cmd fields that have '#' are not to be sent to the sdr
struct field main_controls[] = {
	{R1_FREQ, do_tuning, 600, 0, 150, 49, "FREQ", 5, "14000000", FIELD_NUMBER, FONT_SMALL, NULL, 500000, 30000000, 100},

	// Main RX
	{R1_VOLUME, NULL, 750, 330, 50, 50, "AUDIO", 40, "60", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0, 100, 1},
	{R1_MODE, NULL, 500, 330, 50, 50, "MODE", 40, "USB", FIELD_SELECTION, FONT_FIELD_VALUE,  MODE_SEL, 0,0, 0},
	{R1_LOW, NULL, 550, 330, 50, 50, "LOW", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,4000, 50},
	{R1_HIGH, NULL, 600, 330, 50, 50, "HIGH", 40, "3000", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 300, 10000, 50},

	{R1_AGC, NULL, 650, 330, 50, 50, "AGC", 40, "SLOW", FIELD_SELECTION, FONT_FIELD_VALUE,  AGC_SEL, 0, 1024, 1},
	{R1_GAIN, NULL, 700, 330, 50, 50, "IF", 40, "60", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 0, 100, 1},

	// tx 
	{TX_POWER, NULL, 550, 430, 50, 50, "DRIVE", 40, "40", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 100, 1},
	{TX_GAIN, NULL, 550, 380, 50, 50, "MIC", 40, "50", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0, 100, 1},

	{_SPLIT, NULL, 750, 380, 50, 50, "SPLIT", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{TX_COMPRESS, NULL, 600, 380, 50, 50, "COMP", 40, "0", FIELD_NUMBER, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,100,10},
	{_RIT, NULL, 550, 0, 50, 50, "RIT", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{_TX_WPM, NULL, 650, 380, 50, 50, "WPM", 40, "12", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 50, 1},
	{RX_PITCH, do_pitch, 700, 380, 50, 50, "PITCH", 40, "600", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100, 3000, 10},
	
	{_TX, NULL, 1000, -1000, 50, 50, "TX", 40, "", FIELD_BUTTON, FONT_FIELD_VALUE, RX_TX_SEL, 0,0, 0},

	{_RX, NULL, 650, 430, 50, 50, "RX", 40, "", FIELD_BUTTON, FONT_FIELD_VALUE,  RX_TX_SEL, 0,0, 0},
	
	{_RECORD, do_record, 700, 430, 50, 50, "REC", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0, 0},

	// top row
	{_STEP, NULL, 400, 0 ,50, 50, "STEP", 1, "10Hz", FIELD_SELECTION, FONT_FIELD_VALUE, TUNE_STEP_SEL, 0,0,0},
	{_VFO, NULL, 450, 0 ,50, 50, "VFO", 1, "A", FIELD_SELECTION, FONT_FIELD_VALUE, A_B_SEL, 0,0,0},
	{_SPAN, NULL, 500, 0 ,50, 50, "SPAN", 1, "24K", FIELD_SELECTION, FONT_FIELD_VALUE,  SPECT_WIDTH_SEL, 0,0,0},

	{SPECTRUM, do_spectrum, 400, 80, 400, 100, "Spectrum ", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL, NULL, 0,0,0},  
	{_STATUS, do_status, 400, 51, 400, 29, "STATUS", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL,  STATUS_SEL, 0,0,0},  
	{WATERFALL, do_waterfall, 400, 180 , 400, 149, "Waterfall ", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL,  NULL, 0,0,0},
	{_CONSOLE, do_console, 0, 0 , 400, 320, "CONSOLE", 70, "console box", FIELD_CONSOLE, FONT_LOG,  NOTHING_SEL, 0,0,0},
	{_LOG_ED, NULL, 0, 320, 400, 20, "", 70, "", FIELD_STATIC, FONT_LOG, NOTHING_SEL, 0,128,0},
	{_TEXT_IN, do_text, 0, 340, 398, 20, "TEXT", 70, "text box", FIELD_TEXT, FONT_LOG, NOTHING_SEL, 0,128,0},

	{_CLOSE, NULL, 750, 430 ,50, 50, "_", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_OFF, NULL, 750, 0 ,50, 50, "x", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
  
	// other settings - currently off screen
	{REVERSE_SCROLLING, NULL, 1000, -1000, 50, 50, "RS", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0,0},
	{TUNING_ACCELARATION, NULL, 1000, -1000, 50, 50, "TA", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0,0},
	{TUNING_ACCEL_THRESH1, NULL, 1000, -1000, 50, 50, "TAT1", 40, "10000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100,99999,100},
	{TUNING_ACCEL_THRESH2, NULL, 1000, -1000, 50, 50, "TAT2", 40, "500", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100,99999,100},
	{MOUSE_POINTER, NULL, 1000, -1000, 50, 50, "MP", 40, "LEFT", FIELD_SELECTION, FONT_FIELD_VALUE, MOUSE_SEL, 0,0,0},


	//moving global variables into fields 	
	{_VFO_A_FREQ, NULL, 1000, -1000, 50, 50, "VFOA", 40, "14000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 500000,30000000,1},
	{_VFO_B_FREQ, NULL, 1000, -1000, 50, 50, "VFOB", 40, "7000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 500000,30000000,1},
	{_RIT_DELTA, NULL, 1000, -1000, 50, 50, "RIT_DELTA", 40, "000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, -25000,25000,1},
	{_MYCALLSIGN, NULL, 1000, -1000, 400, 149, "MYCALLSIGN", 70, "NOBODY", FIELD_TEXT, FONT_SMALL,  NULL, 3,10,1},
	{_MYGRID, NULL, 1000, -1000, 400, 149, "MYGRID", 70, "NOWHERE", FIELD_TEXT, FONT_SMALL,  NULL, 4,6,1},
	{_CWINPUT, NULL, 1000, -1000, 50, 50, "CW_INPUT", 40, "KEYBOARD", FIELD_SELECTION, FONT_FIELD_VALUE, KEYER_SEL, 0,0,0},
	{_CWDELAY, NULL, 1000, -1000, 50, 50, "CW_DELAY", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 50, 1000, 50},
	{_TX_PITCH, NULL, 1000, -1000, 50, 50, "TX_PITCH", 40, "600", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 300, 3000, 10},
	{SIDETONE, NULL, 1000, -1000, 50, 50, "SIDETONE", 40, "25", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 0, 100, 5},
	{_CONTACT_CALLSIGN, NULL, 1000, -1000, 400, 149, "", 70, "NOBODY", FIELD_TEXT, FONT_SMALL,  NULL, 3,10,1},
	{_SENT_EXCHANGE, NULL, 1000, -1000, 400, 149, "", 70, "", FIELD_TEXT, FONT_SMALL,  NULL, 0,10,1},
	{_CONTEST_SERIAL, NULL, 1000, -1000, 50, 50, "CONTEST_SERIAL", 40, "0", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,1000000,1},
	{_CURENT_MACRO, NULL, 1000, -1000, 400, 149, "MACRO", 70, "", FIELD_TEXT, FONT_SMALL,  NULL, 0,32,1},
	{_FWDPOWER, NULL, 1000, -1000, 50, 50, "POWER", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,10000,1},
	{_VSWR, NULL, 1000, -1000, 50, 50, "REF", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,10000, 1},
	{BRIDGE, NULL, 1000, -1000, 50, 50, "BRIDGE", 40, "100", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 10,100, 1},

	// FT8 should be 4000 Hz
	{_BW_VOICE, NULL, 1000, -1000, 50, 50, "BW_VOICE", 40, "2200", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},
	{_BW_CW, NULL, 1000, -1000, 50, 50, "BW_CW", 40, "400", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},
	{_BW_DIGITAL, NULL, 1000, -1000, 50, 50, "BW_DIGITAL", 40, "3000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},

	// FT8 controls
	{_FT8_AUTO, NULL, 1000, -1000, 50, 50, "FT8_AUTO", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{_FT8_TX1ST, NULL, 1000, -1000, 50, 50, "FT8_TX1ST", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
    {_FT8_REPEAT, NULL, 1000, -1000, 50, 50, "FT8_REPEAT", 40, "5", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 10, 1},
	
	{_PASSKEY, NULL, 1000, -1000, 400, 149, "PASSKEY", 70, "123", FIELD_TEXT, FONT_SMALL,  NULL, 0,32,1},
	{_TELNETURL, NULL, 1000, -1000, 400, 149, "TELNETURL", 70, "dxc.nc7j.com:7373", FIELD_TEXT, FONT_SMALL, NULL, 0,32,1},


	/* band stack registers */
	{_10M, NULL, 400, 330, 50, 50, "10M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_12M, NULL, 450, 330, 50, 50, "12M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_15M, NULL, 400, 380, 50, 50, "15M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_17M, NULL, 450, 380, 50, 50, "17M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_20M, NULL, 500, 380, 50, 50, "20M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_30M, NULL, 400, 430, 50, 50, "30M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_40M, NULL, 450, 430, 50, 50, "40M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{_80M, NULL, 500, 430, 50, 50, "80M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},

	// soft keyboard
	{_KBD_Q, do_kbd,   0, 360 ,40, 30, "#", 1, "q", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_W, do_kbd,  40, 360, 40, 30, "1", 1, "w", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_E, do_kbd,  80, 360, 40, 30, "2", 1, "e", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_R, do_kbd, 120, 360, 40, 30, "3", 1, "r", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_T, do_kbd, 160, 360, 40, 30, "(", 1, "t", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_Y, do_kbd, 200, 360, 40, 30, ")", 1, "y", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_U, do_kbd, 240, 360, 40, 30, "_", 1, "u", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_I, do_kbd, 280, 360, 40, 30, "-", 1, "i", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_O, do_kbd, 320, 360, 40, 30, "+", 1, "o", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_P, do_kbd, 360, 360, 40, 30, "@", 1, "p", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	{_KBD_A, do_kbd,   0, 390 ,40, 30, "*", 1, "a", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_S, do_kbd,  40, 390, 40, 30, "4", 1, "s", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_D, do_kbd,  80, 390, 40, 30, "5", 1, "d", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_F, do_kbd, 120, 390, 40, 30, "6", 1, "f", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_G, do_kbd, 160, 390, 40, 30, "/", 1, "g", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_H, do_kbd, 200, 390, 40, 30, ":", 1, "h", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_J, do_kbd, 240, 390, 40, 30, ";", 1, "j", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_K, do_kbd, 280, 390, 40, 30, "'", 1, "k", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_L, do_kbd, 320, 390, 40, 30, "\"", 1, "l", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_BS, do_kbd, 360, 390, 40, 30, "", 1, "DEL", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0},
 
	{_KBD_ALT, do_kbd, 0, 420 ,40, 30, "", 1, "Alt", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_Z, do_kbd, 40, 420, 40, 30, "7", 1, "z", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_X, do_kbd, 80, 420, 40, 30, "8", 1, "x", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_C, do_kbd, 120, 420, 40, 30, "9", 1, "c", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_V, do_kbd, 160, 420, 40, 30, "?", 1, "v", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_B, do_kbd, 200, 420, 40, 30, "!", 1, "b", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_N, do_kbd, 240, 420, 40, 30, ",", 1, "n", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_M, do_kbd, 280, 420, 40, 30, ".", 1, "m", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_ENTER, do_kbd, 320, 420, 80, 30, "", 1, "Enter", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	{_KBD_CMD, do_kbd, 0, 450, 80, 30, "", 1, "\\cmd", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_0, do_kbd, 80, 450, 40, 30, "", 1, "0", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_SPACE, do_kbd, 120, 450, 120, 30, "", 1, " SPACE ", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_PERIOD, do_kbd, 240, 450, 40, 30, "\"", 1, ".", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_QUESTION, do_kbd, 280, 450, 40, 30, "?", 1, "?", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_KBD_MACRO, do_kbd, 320, 450, 80, 30, "", 1, "Macro", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 


	// macros keyboard

	// row 1
	{_MF1, do_macro,   0, 1360, 65, 40, "F1", 1, "CQ", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF2, do_macro,  65, 1360, 65, 40, "F2", 1, "Call", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF3, do_macro, 130, 1360, 65, 40, "F3", 1, "Reply", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF4, do_macro, 195, 1360, 65, 40, "F4", 1, "RRR", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF5, do_macro, 260, 1360, 70, 40, "F5", 1, "73", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF6, do_macro, 330, 1360, 70, 40, "F6", 1, "Call", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	// row 2
	{_MF7,  do_macro,   0, 1400, 65, 40, "F7", 1, "Exch", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF8,  do_macro,  65, 1400, 65, 40, "F8", 1, "Tu", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF9,  do_macro, 130, 1400, 65, 40, "F9", 1, "Rpt", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF10, do_macro, 195, 1400, 65, 40, "F10", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF11, do_macro, 260, 1400, 70, 40, "F11", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MF12, do_macro, 330, 1400, 70, 40, "F12", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0},

	// row 3
	{_MFQRZ,  do_macro,   0, 1440, 65, 40, "QRZ", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MFWIPE, do_macro,  65, 1440, 65, 40, "Wipe", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MFLOG,  do_macro, 130, 1440, 65, 40, "Log it", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MFEDIT, do_macro, 195, 1440, 65, 40, "Edit", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MFSPOT, do_macro, 260, 1440, 70, 40, "Spot", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{_MFKBD,  do_macro, 330, 1440, 70, 40, "Kbd", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	//the last control has empty cmd field 
	{"", NULL, 0, 0 ,0, 0, "#", 1, "Q", FIELD_BUTTON, FONT_FIELD_VALUE, NULL, 0,0,0},
};

struct field *active_layout = NULL;

struct field *get_field(const char * const cmd){
	for (int i = 0; active_layout[i].cmd[0] > 0; i++)
		if (!strcmp(active_layout[i].cmd, cmd))
			return active_layout + i;
	return NULL;
}

void update_field(struct field *f){
	if (f->y >= 0)
		f->is_dirty = 1;
	f->update_remote = 1;
} 

struct field *get_field_by_label(const char * const label){
	for (int i = 0; active_layout[i].cmd[0] > 0; i++)
		if (!strcasecmp(active_layout[i].label, label))
			return active_layout + i;
	return NULL;
}

int get_field_value(const char * const cmd, char *value){
	struct field *f = get_field(cmd);
	if (!f)
		return -1;
	strcpy(value, f->value);
	return 0;
}

int get_field_value_by_label(const char * const label, char *value){
	struct field *f = get_field_by_label(label);
	if (!f)
		return -1;
	strcpy(value, f->value);
	return 0;
}

int remote_update_field(int i, char *text){
	struct field * f = active_layout + i;

	if (f->cmd[0] == 0)
		return -1;
	
	// always send status afresh
	if (!strcmp(f->label, "STATUS")){
		// send time
		time_t now = time(NULL);
		struct tm *tmp = gmtime(&now);
		sprintf(text, "STATUS %04d/%02d/%02d %02d:%02d:%02dZ",  
			tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec); 
		return 1;
	}

	strcpy(text, f->label);
	strcat(text, " ");
	strcat(text, f->value);
	int update = f->update_remote;
	f->update_remote = 0;

	// debug on
    //	if (!strcmp(f->cmd, "#text_in") && strlen(f->value))
    //		printf("#text_in [%s] %d\n", f->value, update);
	// debug off
	return update;
}

// set the field directly to a particular value, programmatically
int set_field(const char * const id, char *value){
	struct field *f = get_field(id);
	int v;
	int debug = 0;

	if (!f){
		printf("*Error: field[%s] not found. Check for typo?\n", id);
		return 1;
	}
	
	if (f->value_type == FIELD_NUMBER){
		int	v = atoi(value);
		if (v < f->min)
			v = f->min;
		if (v > f->max)
			v = f->max;
		sprintf(f->value, "%d",  v);
	}
	else if (f->value_type == FIELD_SELECTION || f->value_type == FIELD_TOGGLE){
		// toggle and selection are the same type: toggle has just two values instead of many more
		char *p, *prev, *next, b[100];
		// search the current text in the selection
		prev = NULL;
		if (debug)
			printf("field selection [%s]\n");
		strcpy(b, f->selection);
		p = strtok(b, "/");
		if (debug)
			printf("first token [%s]\n", p);
		while(p){
			if (!strcmp(value, p))
				break;
			else
				prev = p;
			p = strtok(NULL, "/");
			if (debug)
				printf("second token [%s]\n", p);
		}	
		// set to the first option
		if (p == NULL){
			if (prev)
				strcpy(f->value, prev);
			printf("*Error: setting field[%s] to [%s] not permitted\n", f->cmd, value);
			return 1;
		}
		else{
			if (debug)
				printf("Setting field to %s\n", value);
			strcpy(f->value, value);
		}
	}
	else if (f->value_type == FIELD_BUTTON){
		strcpy(f->value, value);	
		return 1;
	}
	else if (f->value_type == FIELD_TEXT){
		if (strlen(value) > f->max || strlen(value) < f->min){
			printf("*Error: field[%s] can't be set to [%s], improper size.\n", f->cmd, value);
			return 1;
		}
		else
			strcpy(f->value, value);
	}

	if (!strcmp(id, "#rit") || !strcmp(id, "#ft8_auto"))
		debug = 1; 

	// send a command to the radio 
	char buff[200];
	sprintf(buff, "%s=%s", f->cmd, f->value);
	do_cmd(buff);
	update_field(f);
	return 0;
}

bool parseEqual(const char const *text, char *left, char *right) {
    char *found = strchr(text, '=');
    if (found) {
        memcpy(left, text, found - text);
        left[found - text + 1] = '\0';
        strcpy(right, found + 1);
        return true;
    } else {
        return false;
    }
}


