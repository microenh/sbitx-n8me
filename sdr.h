#pragma once

#include <complex.h>
#include <fftw3.h>
#include <stdint.h>

/*
Overview:
The SDR's core is quite simple:
We convert the entire sampled band into frequency domain (the stuff you see in the
waterfalls) by simply passing the I and Q samples to to a library called the FFT3W.

As the FFT turns time samples signals into their frequency bins, it the signals
are spread on both sides of the 0 hz line. 

The FFT lines up not just amplitudes of signals at each frequency but also their phase.
A reverse FFT will add up all these sinewaves and reproduce the signal.

To bring an signal to baseband,we just rotate the bins around until the carrier of 
the SSB signal is at 0 bin. Then, we filter out the rest.

The filter is very simple, we just draw the filter shape in frequency domain
and multiply the FFT bin with that.
 
Once, we are left just the baseband signal in the FFT, we do an inverse FFT
and convert the signals back to time domain.

The basic receiver is just that.

HOW THE FILTER WORKS
This is a little tricky, it took me some time to understand.

To begin with understand that you can convert a series of time samples of a signal
to frequency domain and convert it back to get back the original signal.

Time and Frequency domains are just two ways to represent a signal. We hams are
find it more convenient to handle frequency domain.

We tune to a band, we schedule calls on some, we scan, etc. We all understand 
the waterfall.

So here goes:

1. When you see a single spike on the waterfall, it means there is a cw signal.
When you see the time samples, you will see a continuous sinewave. 

2. Similarly, if there is a blip in the time samples, it spreads across the entire
waterfall.

If you stare at the two above statements, you will realize that what appears as a 
singe sample standing out in one domain corresponds to a a continuous response
in the other.

So, if we convert a signal to frequeny domain, zero all the bins except the
the frequency that we intend to hear, convert it back to time domain and
play it through the speaker? It will work, almost. But not really. The reason is
that some signals fall between two bins. These and other signal types will generate
all kinds of artifacts and clicks. You need a filter that is smooth.
There are ways to smoothen it. 

I am following a method that Phil Karn suggested to me. I first produce a 
'perfect filter' in frequency domain by setting frequency bins of the frequency
that I need to '1' and the rest to zeros. Then, I convert this back to time domain using 
inverse FFT. Now, if you think about the point 1 and 2 made above, you will
can guess that the time domain representation of the filter's shape will
have continuos waveform. 

*/

#define MAX_BINS 2048
#define RX_SAMPLE_RATE 96000

// high or low side for inversion for LSB
#define LO_USB 40035000
#define LO_LSB 39987000

/*
All the incoming samples are converted to frequency domain in sound_process(). 
The fft_out stores these as frequency bins.
These are fft_out bins are also used to paint the spectrum and waterfall.

You can have any number of radios working with different slices of the spectrum.
At the moment, only ssb (and CW as a single sideband signal) are demodulated.
Each receiver is inserted a node in a linked list that starts at rx_list.

Each receiver is defined by the struct rx.
Each receiver copies the fft_bins to by shifting it around to bring the desired 
to baseband. 

Each tx is also based on a struct rx but it is used to describe and hold state
for the transmission. The data required is the same!
*/

extern float fft_bins[];
// extern int spectrum_plot[];
extern fftwf_complex *fft_spectrum;
extern struct filter *ssb;

// vfo definitions

#define MAX_PHASE_COUNT (16385)
struct vfo {
	int freq_hz;
	int phase;
	int phase_increment;
};

void vfo_init_phase_table();
void vfo_start(struct vfo *v, int frequency_hz, int start_phase);
int vfo_read(struct vfo *v);


// the filter definitions
struct filter {
	fftwf_complex *fir_coeff;
	int N;
	int L;
	int M;
    fftwf_plan fwd;
    fftwf_plan rev;
};

struct filter *filter_new(int input_length, int impulse_length);
int filter_tune(struct filter *f, float const low,float const high,float const kaiser_beta);
int make_hann_window(float *window, int max_count);
void filter_print(struct filter *f);


// Complex norm (sum of squares of real and imaginary parts)
static inline float const cnrmf(const complex float x){
	return crealf(x)*crealf(x) + cimagf(x) * cimagf(x);
}
static inline double const cnrm(const complex double x){
	return creal(x)*creal(x) + cimag(x) * cimag(x);
}

#define power2dB(x) (10*log10f(x))

#define MAX_MODES 11 

enum {
    MODE_USB,
    MODE_LSB,
    MODE_CW,
    MODE_CWR,
    MODE_NBFM,
    MODE_AM,
    MODE_FT8,
    MODE_PSK31,
    MODE_RTTY,
    MODE_DIGITAL,
    MODE_2TONE,
    MODE_CALIBRATE
};

struct rx {
	long tuned_bin;	// tuned bin (this should translate to freq) 
	short mode;		// USB/LSB/AM/FM (cw is narrow SSB, so not listed)
					// FFT plan to convert back to time domain
	int low_hz; 
	int high_hz;
	fftwf_plan plan_rev;
	fftwf_complex *fft_freq;
    float *fft_time;
 
	/*
	* agc() is called once for every block of samples. The samples
	* are in time-domain. Consider each call to agc as a 'tick'.
	* agc_speed is the max ticks that the agc will hang for
	* agc_loop tracks how many more ticks to go before the decay
	* agc_decay rate sets the slope for agc decay.
	*/
	int agc_speed;
	int agc_threshold;
	int agc_loop;
	double signal_strength;
	double agc_gain;
	int agc_decay_rate;
	double signal_avg;

	struct filter *filter;	// convolution filter
	int output;				// -1 = nowhere, 0 = audio, rest is a tcp socket
	struct rx* next;
};

extern struct rx *rx_list;
extern int freq_hdr;

void set_volume(double v);
void sdr_request(char *request, char *response);

/* from modems.c */
void modem_rx(int mode, int32_t *samples, int count);
void modem_set_pitch(int pitch);
void modem_init();
int get_tx_data_byte(char *c);
int	get_tx_data_length();
void modem_poll(int mode);
float modem_next_sample(int mode);
void ft8_tx(char *message, int freq);
void modem_abort();
void ft8_interpret(char *received, char *transmit);

extern int in_tx;

enum {
    TX_OFF,
    TX_PTT,
    TX_SOFT
};

void tx_on(int trigger);
void tx_off();
long get_freq();
int get_pitch();
// void do_cmd(char *cmd);

// cw defines, these are bitfields, hence, powers of 2
#define CW_IDLE (0)
#define CW_DASH (1) 
#define CW_DOT (2) 
#define CW_DOT_DELAY (4)
#define CW_DASH_DELAY (8) 
#define CW_WORD_DELAY (16) 
#define CW_DOWN (32) 

// straight key, iambic, keyboard
#define CW_STRAIGHT 0
#define CW_IAMBIC	1
#define CW_IAMBICB 2	
#define CW_KBD 3


int key_poll();
int get_cw_delay();
int get_cw_input_method();
int get_cw_tx_pitch();
int get_modem_pitch();
int	get_wpm();
#define FT8_AUTO 2
#define FT8_SEMI 1
#define FT8_MANUAL 0
void ft8_setmode(int config);

void telnet_open(char *server);
int telnet_write(char *text);
void telnet_close();
FILE *wav_start_writing(const char* path);

