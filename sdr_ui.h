void setup(void);
void loop(void);
int set_field(char *id, char *value);
int get_field_value(char *id, char *value);
int get_field_value_by_label(char *label, char *value);
extern int spectrum_plot[];
void remote_execute(char *command);
int remote_update_field(int i, char *text);
void web_get_spectrum(char *buff);
int web_get_console(char *buff, int max);
int remote_audio_output(int16_t *samples);
int is_in_tx(void);
void abort_tx(void);
extern int display_freq;


void write_console(int style, char *text);
int macro_load(char *filename, char *output);
int macro_exec(int key, char *dest);
void macro_label(int fn_key, char *label);
void macro_list(char *output);
void macro_get_keys(char *output);
time_t time_sbitx();

#define VER_STR "sbitx v2.0 - N8ME"
