#include "command_str.h"

// field commands
const char R1_FREQ[] = "r1:freq";
const char R1_VOLUME[] = "r1:volume";
const char R1_MODE[] = "r1:mode";
const char R1_LOW[] = "r1:low";
const char R1_HIGH[] = "r1:high";
const char R1_AGC[] = "r1:agc";
const char R1_GAIN[] = "r1:gain";
const char TX_POWER[] = "tx_power";
const char TX_GAIN[] = "tx_gain";
const char _SPLIT[] = "#split";
const char TX_COMPRESS[] = "tx_compress";
const char _RIT[] = "#rit";
const char _TX_WPM[] = "#tx_wpm";
const char RX_PITCH[] = "rx_pitch";
const char _TX[] = "#tx";
const char _RX[] = "#rx";
const char _RECORD[] = "#record";
const char _STEP[] = "#step";
const char _VFO[] = "#vfo";
const char _SPAN[] = "#span";
const char SPECTRUM[] = "spectrum";
const char _STATUS[] = "#status";
const char WATERFALL[] = "waterfall";
const char _CONSOLE[] = "#console";
const char _LOG_ED[] = "#log_ed";
const char _TEXT_IN[] = "#text_in";
const char _CLOSE[] = "#close";
const char _OFF[] = "#off";
const char REVERSE_SCROLLING[] = "reverse_scrolling";
const char TUNING_ACCELARATION[] = "tuning_acceleration";
const char TUNING_ACCEL_THRESH1[] = "tuning_accel_thresh1";
const char TUNING_ACCEL_THRESH2[] = "tuning_accel_thresh2";
const char MOUSE_POINTER[] = "mouse_pointer";
const char _VFO_A_FREQ[] = "#vfo_a_freq";
const char _VFO_B_FREQ[] = "#vfo_b_freq";
const char _RIT_DELTA[] = "#rit_delta";
const char _MYCALLSIGN[] = "#mycallsign";
const char _MYGRID[] = "#mygrid";
const char _CWINPUT[] = "#cwinput";
const char _CWDELAY[] = "#cwdelay";
const char _TX_PITCH[] = "#tx_pitch";
const char SIDETONE[] = "sidetone";
const char _CONTACT_CALLSIGN[] = "#contact_callsign";
const char _SENT_EXCHANGE[] = "#sent_exchange";
const char _CONTEST_SERIAL[] = "#contest_serial";
const char _CURENT_MACRO[] = "#current_macro";
const char _FWDPOWER[] = "#fwdpower";
const char _VSWR[] = "#vswr";
const char BRIDGE[] = "bridge";
const char _BW_VOICE[] = "#bw_voice";
const char _BW_CW[] = "#bw_cw";
const char _BW_DIGITAL[] = "#bw_digital";
const char _FT8_AUTO[] = "#ft8_auto";
const char _FT8_TX1ST[] = "#ft8_tx1st";
const char _FT8_REPEAT[] = "#ft8_repeat";
const char _PASSKEY[] = "#passkey";
const char _TELNETURL[] = "#telneturl";
const char _10M[] = "#10m";
const char _12M[] = "#12m";
const char _15M[] = "#15m";
const char _17M[] = "#17m";
const char _20M[] = "#20m";
const char _30M[] = "#30m";
const char _40M[] = "#40m";
const char _80M[] = "#80m";
const char _KBD_Q[] = "#kbd_q";
const char _KBD_W[] = "#kbd_w";
const char _KBD_E[] = "#kbd_e";
const char _KBD_R[] = "#kbd_r";
const char _KBD_T[] = "#kbd_t";
const char _KBD_Y[] = "#kbd_y";
const char _KBD_U[] = "#kbd_u";
const char _KBD_I[] = "#kbd_i";
const char _KBD_O[] = "#kbd_o";
const char _KBD_P[] = "#kbd_p";
const char _KBD_A[] = "#kbd_a";
const char _KBD_S[] = "#kbd_s";
const char _KBD_D[] = "#kbd_d";
const char _KBD_F[] = "#kbd_f";
const char _KBD_G[] = "#kbd_g";
const char _KBD_H[] = "#kbd_h";
const char _KBD_J[] = "#kbd_j";
const char _KBD_K[] = "#kbd_k";
const char _KBD_L[] = "#kbd_l";
const char _KBD_BS[] = "#kbd_bs";
const char _KBD_ALT[] = "#kbd_alt";
const char _KBD_Z[] = "#kbd_z";
const char _KBD_X[] = "#kbd_x";
const char _KBD_C[] = "#kbd_c";
const char _KBD_V[] = "#kbd_v";
const char _KBD_B[] = "#kbd_b";
const char _KBD_N[] = "#kbd_n";
const char _KBD_M[] = "#kbd_m";
const char _KBD_ENTER[] = "#kbd_enter";
const char _KBD_CMD[] = "#kbd_cmd";
const char _KBD_0[] = "#kbd_0";
const char _KBD_SPACE [] = "#kbd_ ";
const char _KBD_PERIOD[] = "#kbd_.";
const char _KBD_QUESTION[] = "#kbd_?";
const char _KBD_MACRO[] = "#kbd_macro";
const char _MF1[] = "#mf1";
const char _MF2[] = "#mf2";
const char _MF3[] = "#mf3";
const char _MF4[] = "#mf4";
const char _MF5[] = "#mf5";
const char _MF6[] = "#mf6";
const char _MF7[] = "#mf7";
const char _MF8[] = "#mf8";
const char _MF9[] = "#mf9";
const char _MF10[] = "#mf10";
const char _MF11[] = "#mf11";
const char _MF12[] = "#mf12";
const char _MFQRZ[] = "#mfqrz";
const char _MFWIPE[] = "#mfwipe";
const char _MFLOG[] = "#mflog";
const char _MFEDIT[] = "#mfedit";
const char _MFSPOT[] = "#mfspot";
const char _MFKBD[] = "#mfkbd";
