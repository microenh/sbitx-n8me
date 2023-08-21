#include <cairo.h>
#include <stddef.h>

#include "fields.h"
#include "fonts.h"

const char MODE_SEL[] = "USB/LSB/CW/CWR/FT8/PSK31/RTTY/DIGITAL/2TONE";
const char AGC_SEL[] = "OFF/SLOW/MED/FAST";
const char ON_OFF_SEL[] = "ON/OFF";
const char RX_TX_SEL[] = "RX/TX";
const char STATUS_SEL[] = "status";
const char TUNE_STEP_SEL[] = "1M/100K/10K/1K/100H/10H";
const char A_B_SEL[] = "A/B";
const char SPECT_WIDTH_SEL[] = "25K/10K/6K/2.5K";
const char NOTHING_SEL[] = "nothing valuable";
const char MOUSE_SEL[] = "BLANK/LEFT/RIGHT/CROSSHAIR";
const char KEYER_SEL[] = "KEYBOARD/IAMBIC/IAMBICB/STRAIGHT";

// the cmd fields that have '#' are not to be sent to the sdr
struct field main_controls[] = {
	{"r1:freq", do_tuning, 600, 0, 150, 49, "FREQ", 5, "14000000", FIELD_NUMBER, FONT_SMALL, NULL, 500000, 30000000, 100},

	// Main RX
	{"r1:volume", NULL, 750, 330, 50, 50, "AUDIO", 40, "60", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0, 100, 1},
	{"r1:mode", NULL, 500, 330, 50, 50, "MODE", 40, "USB", FIELD_SELECTION, FONT_FIELD_VALUE,  MODE_SEL, 0,0, 0},
	{"r1:low", NULL, 550, 330, 50, 50, "LOW", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,4000, 50},
	{"r1:high", NULL, 600, 330, 50, 50, "HIGH", 40, "3000", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 300, 10000, 50},

	{"r1:agc", NULL, 650, 330, 50, 50, "AGC", 40, "SLOW", FIELD_SELECTION, FONT_FIELD_VALUE,  AGC_SEL, 0, 1024, 1},
	{"r1:gain", NULL, 700, 330, 50, 50, "IF", 40, "60", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 0, 100, 1},

	// tx 
	{"tx_power", NULL, 550, 430, 50, 50, "DRIVE", 40, "40", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 100, 1},
	{"tx_gain", NULL, 550, 380, 50, 50, "MIC", 40, "50", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0, 100, 1},

	{"#split", NULL, 750, 380, 50, 50, "SPLIT", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{"tx_compress", NULL, 600, 380, 50, 50, "COMP", 40, "0", FIELD_NUMBER, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,100,10},
	{"#rit", NULL, 550, 0, 50, 50, "RIT", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{"#tx_wpm", NULL, 650, 380, 50, 50, "WPM", 40, "12", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 50, 1},
	{"rx_pitch", do_pitch, 700, 380, 50, 50, "PITCH", 40, "600", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100, 3000, 10},
	
	{"#tx", NULL, 1000, -1000, 50, 50, "TX", 40, "", FIELD_BUTTON, FONT_FIELD_VALUE, RX_TX_SEL, 0,0, 0},

	{"#rx", NULL, 650, 430, 50, 50, "RX", 40, "", FIELD_BUTTON, FONT_FIELD_VALUE,  RX_TX_SEL, 0,0, 0},
	
	{"#record", do_record, 700, 430, 50, 50, "REC", 40, "OFF", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0, 0},

	// top row
	{"#step", NULL, 400, 0 ,50, 50, "STEP", 1, "10Hz", FIELD_SELECTION, FONT_FIELD_VALUE, TUNE_STEP_SEL, 0,0,0},
	{"#vfo", NULL, 450, 0 ,50, 50, "VFO", 1, "A", FIELD_SELECTION, FONT_FIELD_VALUE, A_B_SEL, 0,0,0},
	{"#span", NULL, 500, 0 ,50, 50, "SPAN", 1, "25K", FIELD_SELECTION, FONT_FIELD_VALUE,  SPECT_WIDTH_SEL, 0,0,0},

	{"spectrum", do_spectrum, 400, 80, 400, 100, "Spectrum ", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL, NULL, 0,0,0},  
	{"#status", do_status, 400, 51, 400, 29, "STATUS", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL,  STATUS_SEL, 0,0,0},  
	{"waterfall", do_waterfall, 400, 180 , 400, 149, "Waterfall ", 70, "7000 KHz", FIELD_STATIC, FONT_SMALL,  NULL, 0,0,0},
	{"#console", do_console, 0, 0 , 400, 320, "CONSOLE", 70, "console box", FIELD_CONSOLE, FONT_LOG,  NOTHING_SEL, 0,0,0},
	{"#log_ed", NULL, 0, 320, 400, 20, "", 70, "", FIELD_STATIC, FONT_LOG, NOTHING_SEL, 0,128,0},
	{"#text_in", do_text, 0, 340, 398, 20, "TEXT", 70, "text box", FIELD_TEXT, FONT_LOG, NOTHING_SEL, 0,128,0},

	{"#close", NULL, 750, 430 ,50, 50, "_", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#off", NULL, 750, 0 ,50, 50, "x", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
  
	// other settings - currently off screen
	{"reverse_scrolling", NULL, 1000, -1000, 50, 50, "RS", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0,0},
	{"tuning_acceleration", NULL, 1000, -1000, 50, 50, "TA", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE, ON_OFF_SEL, 0,0,0},
	{"tuning_accel_thresh1", NULL, 1000, -1000, 50, 50, "TAT1", 40, "10000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100,99999,100},
	{"tuning_accel_thresh2", NULL, 1000, -1000, 50, 50, "TAT2", 40, "500", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 100,99999,100},
	{"mouse_pointer", NULL, 1000, -1000, 50, 50, "MP", 40, "LEFT", FIELD_SELECTION, FONT_FIELD_VALUE, MOUSE_SEL, 0,0,0},


	//moving global variables into fields 	
	{"#vfo_a_freq", NULL, 1000, -1000, 50, 50, "VFOA", 40, "14000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 500000,30000000,1},
	{"#vfo_b_freq", NULL, 1000, -1000, 50, 50, "VFOB", 40, "7000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 500000,30000000,1},
	{"#rit_delta", NULL, 1000, -1000, 50, 50, "RIT_DELTA", 40, "000000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, -25000,25000,1},
	{"#mycallsign", NULL, 1000, -1000, 400, 149, "MYCALLSIGN", 70, "NOBODY", FIELD_TEXT, FONT_SMALL,  NULL, 3,10,1},
	{"#mygrid", NULL, 1000, -1000, 400, 149, "MYGRID", 70, "NOWHERE", FIELD_TEXT, FONT_SMALL,  NULL, 4,6,1},
	{"#cwinput", NULL, 1000, -1000, 50, 50, "CW_INPUT", 40, "KEYBOARD", FIELD_SELECTION, FONT_FIELD_VALUE, KEYER_SEL, 0,0,0},
	{"#cwdelay", NULL, 1000, -1000, 50, 50, "CW_DELAY", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 50, 1000, 50},
	{"#tx_pitch", NULL, 1000, -1000, 50, 50, "TX_PITCH", 40, "600", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 300, 3000, 10},
	{"sidetone", NULL, 1000, -1000, 50, 50, "SIDETONE", 40, "25", FIELD_NUMBER, FONT_FIELD_VALUE,  NULL, 0, 100, 5},
	{"#contact_callsign", NULL, 1000, -1000, 400, 149, "", 70, "NOBODY", FIELD_TEXT, FONT_SMALL,  NULL, 3,10,1},
	{"#sent_exchange", NULL, 1000, -1000, 400, 149, "", 70, "", FIELD_TEXT, FONT_SMALL,  NULL, 0,10,1},
	{"#contest_serial", NULL, 1000, -1000, 50, 50, "CONTEST_SERIAL", 40, "0", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,1000000,1},
	{"#current_macro", NULL, 1000, -1000, 400, 149, "MACRO", 70, "", FIELD_TEXT, FONT_SMALL,  NULL, 0,32,1},
	{"#fwdpower", NULL, 1000, -1000, 50, 50, "POWER", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,10000,1},
	{"#vswr", NULL, 1000, -1000, 50, 50, "REF", 40, "300", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 0,10000, 1},
	{"bridge", NULL, 1000, -1000, 50, 50, "BRIDGE", 40, "100", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 10,100, 1},

	// FT8 should be 4000 Hz
	{"#bw_voice", NULL, 1000, -1000, 50, 50, "BW_VOICE", 40, "2200", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},
	{"#bw_cw", NULL, 1000, -1000, 50, 50, "BW_CW", 40, "400", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},
	{"#bw_digital", NULL, 1000, -1000, 50, 50, "BW_DIGITAL", 40, "3000", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 300, 3000, 50},

	// FT8 controls
	{"#ft8_auto", NULL, 1000, -1000, 50, 50, "FT8_AUTO", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
	{"#ft8_tx1st", NULL, 1000, -1000, 50, 50, "FT8_TX1ST", 40, "ON", FIELD_TOGGLE, FONT_FIELD_VALUE,  ON_OFF_SEL, 0,0,0},
    {"#ft8_repeat", NULL, 1000, -1000, 50, 50, "FT8_REPEAT", 40, "5", FIELD_NUMBER, FONT_FIELD_VALUE, NULL, 1, 10, 1},
	
	{"#passkey", NULL, 1000, -1000, 400, 149, "PASSKEY", 70, "123", FIELD_TEXT, FONT_SMALL,  NULL, 0,32,1},
	{"#telneturl", NULL, 1000, -1000, 400, 149, "TELNETURL", 70, "dxc.nc7j.com:7373", FIELD_TEXT, FONT_SMALL, NULL, 0,32,1},


	/* band stack registers */
	{"#10m", NULL, 400, 330, 50, 50, "10M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#12m", NULL, 450, 330, 50, 50, "12M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#15m", NULL, 400, 380, 50, 50, "15M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#17m", NULL, 450, 380, 50, 50, "17M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#20m", NULL, 500, 380, 50, 50, "20M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#30m", NULL, 400, 430, 50, 50, "30M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#40m", NULL, 450, 430, 50, 50, "40M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},
	{"#80m", NULL, 500, 430, 50, 50, "80M", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,  NULL, 0,0,0},

	// soft keyboard
	{"#kbd_q", do_kbd,   0, 360 ,40, 30, "#", 1, "q", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_w", do_kbd,  40, 360, 40, 30, "1", 1, "w", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_e", do_kbd,  80, 360, 40, 30, "2", 1, "e", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_r", do_kbd, 120, 360, 40, 30, "3", 1, "r", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_t", do_kbd, 160, 360, 40, 30, "(", 1, "t", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_y", do_kbd, 200, 360, 40, 30, ")", 1, "y", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_u", do_kbd, 240, 360, 40, 30, "_", 1, "u", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_i", do_kbd, 280, 360, 40, 30, "-", 1, "i", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_o", do_kbd, 320, 360, 40, 30, "+", 1, "o", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_p", do_kbd, 360, 360, 40, 30, "@", 1, "p", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	{"#kbd_a", do_kbd,   0, 390 ,40, 30, "*", 1, "a", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_s", do_kbd,  40, 390, 40, 30, "4", 1, "s", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_d", do_kbd,  80, 390, 40, 30, "5", 1, "d", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_f", do_kbd, 120, 390, 40, 30, "6", 1, "f", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_g", do_kbd, 160, 390, 40, 30, "/", 1, "g", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_h", do_kbd, 200, 390, 40, 30, ":", 1, "h", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_j", do_kbd, 240, 390, 40, 30, ";", 1, "j", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_k", do_kbd, 280, 390, 40, 30, "'", 1, "k", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_l", do_kbd, 320, 390, 40, 30, "\"", 1, "l", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_bs", do_kbd, 360, 390, 40, 30, "", 1, "DEL", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0},
 
	{"#kbd_alt", do_kbd, 0, 420 ,40, 30, "", 1, "Alt", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_z", do_kbd, 40, 420, 40, 30, "7", 1, "z", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_x", do_kbd, 80, 420, 40, 30, "8", 1, "x", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_c", do_kbd, 120, 420, 40, 30, "9", 1, "c", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_v", do_kbd, 160, 420, 40, 30, "?", 1, "v", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_b", do_kbd, 200, 420, 40, 30, "!", 1, "b", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_n", do_kbd, 240, 420, 40, 30, ",", 1, "n", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_m", do_kbd, 280, 420, 40, 30, ".", 1, "m", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_enter", do_kbd, 320, 420, 80, 30, "", 1, "Enter", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	{"#kbd_cmd", do_kbd, 0, 450, 80, 30, "", 1, "\\cmd", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_0", do_kbd, 80, 450, 40, 30, "", 1, "0", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_ ", do_kbd, 120, 450, 120, 30, "", 1, " SPACE ", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_.", do_kbd, 240, 450, 40, 30, "\"", 1, ".", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_?", do_kbd, 280, 450, 40, 30, "?", 1, "?", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#kbd_macro", do_kbd, 320, 450, 80, 30, "", 1, "Macro", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 


	// macros keyboard

	// row 1
	{"#mf1", do_macro,   0, 1360, 65, 40, "F1", 1, "CQ", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf2", do_macro,  65, 1360, 65, 40, "F2", 1, "Call", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf3", do_macro, 130, 1360, 65, 40, "F3", 1, "Reply", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf4", do_macro, 195, 1360, 65, 40, "F4", 1, "RRR", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf5", do_macro, 260, 1360, 70, 40, "F5", 1, "73", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf6", do_macro, 330, 1360, 70, 40, "F6", 1, "Call", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	// row 2
	{"#mf7",  do_macro,   0, 1400, 65, 40, "F7", 1, "Exch", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf8",  do_macro,  65, 1400, 65, 40, "F8", 1, "Tu", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf9",  do_macro, 130, 1400, 65, 40, "F9", 1, "Rpt", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf10", do_macro, 195, 1400, 65, 40, "F10", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf11", do_macro, 260, 1400, 70, 40, "F11", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mf12", do_macro, 330, 1400, 70, 40, "F12", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0},

	// row 3
	{"#mfqrz",  do_macro,   0, 1440, 65, 40, "QRZ", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mfwipe", do_macro,  65, 1440, 65, 40, "Wipe", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mflog",  do_macro, 130, 1440, 65, 40, "Log it", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mfedit", do_macro, 195, 1440, 65, 40, "Edit", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mfspot", do_macro, 260, 1440, 70, 40, "Spot", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 
	{"#mfkbd",  do_macro, 330, 1440, 70, 40, "Kbd", 1, "", FIELD_BUTTON, FONT_FIELD_VALUE,NULL, 0,0,0}, 

	//the last control has empty cmd field 
	{"", NULL, 0, 0 ,0, 0, "#", 1, "Q", FIELD_BUTTON, FONT_FIELD_VALUE, NULL, 0,0,0},
};
