/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Implementations for the synthesizer modules
 *
 * $Rev$
 * $Date$
 */

#ifndef __MODULES_H__
#define __MODULES_H__

#include "constants.h"

// visual settings
#define 	MODULE_SIZE 60.0f
#define 	MODULE_HALF (MODULE_SIZE/2)
#define 	MODULE_QUARTER (MODULE_SIZE/4)+0.5
#define 	MODULE_FONT GLUT_BITMAP_HELVETICA_10
#define 	FONTHEIGHT 13
#define 	KNOB_RADIUS 10.0f
#define 	NODE_RADIUS 3.0f
#define 	OUTPUT_OFFSET -12.0f

// template macro for module update functions
#define 	MODULE_FUNC(X)	float modfunc_ ##X (unsigned char v, float *mod, void *data, float *ms)

// module types defined
#define 	MODTYPES		16

// knob scales
#define		KNOBSCALES		9

// waveforms and filter types
#define 	VCO_WAVEFORMS		4
#define 	LFO_WAVEFORMS		4
#define 	VCF_MODES		4
#define		DELAY_MODES		2

// defines for module type numbers
#define 	MOD_CV			0
#define 	MOD_ADSR		1
#define 	MOD_WAVEFORM		2
#define 	MOD_LFO			3
#define 	MOD_KNOB		4
#define 	MOD_AMPLIFIER		5
#define 	MOD_MIXER		6
#define 	MOD_FILTER		7
#define 	MOD_LPF24		8
#define 	MOD_DELAY		9
#define		MOD_ATTENUATOR		10
#define 	MOD_RESAMPLE		11
#define 	MOD_SWITCH		12
#define 	MOD_DISTORT		13
#define 	MOD_ACCENT		14
#define 	MOD_OUTPUT		15


// oscillator waveform type defines
#define		VCO_PULSE        0
#define		VCO_SAW          1
#define		VCO_TRIANGLE     2
#define		VCO_SINE         3

// vcf filtering modes
#define		VCF_OFF		0
#define		VCF_LOWPASS	1
#define		VCF_HIGHPASS	2
#define		VCF_BANDPASS	3

// lfo waveforms
#define		LFO_SQUARE	0
#define		LFO_SAW		1
#define		LFO_TRIANGLE	2
#define		LFO_SINE	3

// delay types
#define		DELAY_COMB	0
#define		DELAY_ALLPASS	1

// knob scale types
#define		SCALE_RAW		0
#define		SCALE_FREQUENCY_HZ	1
#define		SCALE_FREQUENCY_TEMPO	2
#define		SCALE_DURATION		3
#define		SCALE_DURATION_TEMPO	4
#define		SCALE_RAMP		5
#define		SCALE_MIDI_NOTE		6
#define		SCALE_NOTE_INTERVAL	7
#define		SCALE_PERCENTAGE	8


// names for the knob scale types
static char *knobScaleNames[KNOBSCALES]=
{
  "Raw float",
  "Frequency (Hz)",
  "Freq from tempo",
  "Duration (sec)",
  "Duration from tempo",
  "Ramp (sec)",
  "mIDI Note",
  "Semitones interval",
  "Percentage"
};
static char *knobScaleUnits[KNOBSCALES]=
{
  "",
  "Hz",
  "x tempo (Hz)",
  "sec",
  "x tempo (sec)",
  "sec",
  "",
  "semitones",
  "%"
};


// Descriptive names for all 16 module types
static char *modTypeNames[MODTYPES]=
{
	"CV",
	"ENV",
	"VCO",
	"LFO",
	"Knob",
	"Amp mod",
	"mixer",
	"VCF",
	"LPF",
	"Delay",
	"Scale",
	"resample",
	"Supersaw", //Switch",
	"Distort",
	"Accent",
	"Output"
};

// Module descriptions
static char *modTypeDescriptions[MODTYPES]=
{
	"Keyboard control voltage (CV) from the sequencer for setting the VCO frequency. The voltage\nis 1 unit/hz, so an A-3 generates a voltage of 440.",
	"ADSR envelope generator with linear attack and decay/release ramps. Sustain level sets\nthe amplification, A/D/R ramps are set as a duration",
	"Voltage controlled oscillator (VCO). Outputs the chosen waveform at a frequency set by the\ncontrol voltage. Includes a noise generator and -1 octave pulse suboscillator with levels\nselectable using control inputs.",
	"The low frequency oscillator (LFO) can be used to, for example, make long filter- and pulse width\nsweeps, vibrato or other effects like chorus and flanging.",
	"Knobs output a control voltage which can be used to control the other modules. There is\nno forced voltage range but 0 to 1 is normally used.",
	"Amplitude modulation amplifies or attenuates the input signal according to the control voltage.\nSince this is done by multiplying, it also works as a ring modulator.",
	"Mixes up to four signals together by adding. Works also an an amplifier or a frequency multiplier\nby connecting the same signal to multiple inputs",
	"State-variable voltage controlled filter (VCF). Resonant low-, band- and highpass filter with\n12db/oct attenuation.",
	"Voltage controlled 24db/oct 4-pole resonant lowpass filter (LPF). Self-oscillates when resonance\nis set to 100%.",
	"Delay module which can operate either as a comb filter or an allpass filter. Has control inputs for\ngain/feedback, loop length and delay time. When feedback is not connected or zero, it functions\nas a normal one-tap variable delay. The loop length is 3 seconds when no control input is provided.",
	"Scaler knobs can be used attenuate or amplify an incoming signal by multiplying it\nwith the modulator value.",
        "Sample-and-hold resamples the signal down by sampling it at given intervals and\noutputting the sampled value until a new one is read. The sample rate input is a frequency in hz",
        "Seven-oscillator supersaw generator with detune and mix controls. Emulates closely\nthe output of a Roland JP8000/JP8080 supersaw.",
	"Amplifies and distorts the input signal by a variable amount",
	"The accent modules outputs the user-specified voltage when a sequencer note\nbeing played has the accent mark. It can be used to change the volume, resonance or any other feature of the note.",
	"Outputs the signal to the master mixer at the selected output level",

};


// number of dwords a module requires for buffer. ptr goes always to
// first dword of the localdata
static int modDataBufferLength[MODTYPES]={
	0, //CV
	0, //ADSR
	0, //wave
	0, //lfo
	0, //knob
	0, //amp
	0, //mixer
	0, //filter
	0, //lpf24
	5*OUTPUTFREQ, //delay
	0, //scaler
	0, //compressor
	0, //switch
	0, //distort
	0, //accent
	0  //output
};


// Number of input nodes on modules
static int modInputCount[MODTYPES]={
	0, //CV
	4, //ADSR (attack, decay, sustain, release)
	4, //wave (frequency, pwm, subosc, noise)
	3, //lfo (frequency, ampl, bias)
	0, //knob
	2, //ampmod (input signal, amplification)
	4, //mixer (four input signals)
	3, //filter (input signal, cutoff, resonance)
	3, //lpf24 (input signal, cutoff, resonance)
	4, //delay (input signal, delay, loop)
	1, //scaler (input signal)
	2, //resample (input signal, samplerate)
	3, //supersaw (pitch, detune, mix)      //4, //switch
	2, //distort (input signal, distortion)
	0, //accent
	1  //output (input signal)
};


// Labels for all input nodes on modules
static char* modInputNames[MODTYPES][4]={
{"","","",""},
{"A", "d", "s", "r"},
{"frEq", "pwm", "sub", "noise"},
{"frEq", "AmpL", "bias", ""},
{"","","",""},
{"in", "Amp", "", ""},
{"in1", "in2", "in3", "in4"},
{"in", "fc", "rEs", ""},
{"in", "fc", "rEs", ""},
{"in", "timE", "loop", "fb"},
{"in", "", "", ""},
{"in", "rate", "", ""},
{"frEq", "detune", "mix"}, //{"in1", "in2", "in3", "in4"},
{"in", "dist", "", ""},
{"","","",""},
{"in", "", "", ""}
};

// expected scale of each input on the modules
/*
static int modInputScale[MODTYPES][4]={
{0, 0, 0, 0},
{SCALE_RAMP, SCALE_RAMP, SCALE_RAMP, SCALE_RAMP},
{SCALE_FREQUENCY, 0, 0, 0},
{SCALE_FREQUENCY, 0, 0, 0},
{0, 0, 0, 0},
{"in", "Amp", "", ""},
{"in1", "in2", "in3", "in4"},
{"in", SCALE_FREQUENCY, "rEs", ""},
{"in", SCALE_FREQUENCY, "rEs", ""},
{"in", SCALE_DURATION, SCALE_DURATION, "fb"},
{"in", "", "", ""},
{"in", SCALE_FREQUENCY, "", ""},
{"in1", "in2", "in3", "in4"},
{"in", "dist", "", ""},
{"","","",""},
{"in", "", "", ""}
};
*/


// modulator names for each module
static char* modModulatorNames[MODTYPES]={
"pitch",
"",
"waveform",
"waveform",
"mod",
"",
"",
"mode",
"",
"mode",
"LeveL",
"",
"input",
"",
"velocity",
"level"
};

// modulator value type. 0=no modulator, 1=float, 2=integer
static int modModulatorTypes[MODTYPES]={
	1, //CV
	0, //ADSR
	3, //wave
	4, //lfo
	1, //knob
	0, //amp
	0, //mixer
	5, //filter
	0, //resample
	6, //delay
	1, //attenuator
	0, //resample
	0, //supersaw    //2, //switch
	0, //distort
	1, //accent
	1  //output
};


static char* modVcoWaveforms[VCO_WAVEFORMS]={"Pulse", "Saw", "Triangle", "Sine"}; //, "Noise"};
static char* modLfoWaveforms[LFO_WAVEFORMS]={"Square", "Saw", "Triangle", "Sine"};
static char* modVcfModes[VCF_MODES]={"Off", "Lowpass", "Highpass", "Bandpass"};
static char* modDelayModes[DELAY_MODES]={"Comb filter", "Allpass filter"};


// input node positions [maxnodes][node]
static float node_xoffset[5][4]={
  {0, 0, 0, 0},
  {-(MODULE_HALF+0.5), 0, 0, 0},
  {0, 0, 0, 0},
  {-(MODULE_HALF+0.5), 0, 0, 0},
  {-MODULE_QUARTER, MODULE_QUARTER, -MODULE_QUARTER, MODULE_QUARTER}
};
static float node_yoffset[5][4]={
  {0, 0, 0, 0},
  {0+OUTPUT_OFFSET, 0, 0, 0},
  {-(MODULE_HALF+0.5), (MODULE_HALF+0.5), 0, 0},
  {0+OUTPUT_OFFSET, -(MODULE_HALF+0.5), (MODULE_HALF+0.5), 0},
  {-(MODULE_HALF+0.5), -(MODULE_HALF+0.5), (MODULE_HALF+0.5), (MODULE_HALF+0.5)}
};

// input node label positions
static float node_labelpos[5][4]={
  {0,0,0,0}, {3,0,0,0}, {0,2,0,0}, {3,0,2,0}, {0,0,2,2}
};
                
// output node positions depending on how many input nodes there are
static int node_outputCount[5]={4, 3, 2, 1, 2};
static int node_outputList[5][4]={
  {0, 1, 2, 3}, {0, 2, 3, -1}, {0, 1, -1, -1}, {0, -1, -1, -1}, {0, 1, -1, -1} };
static int node_outputxoffset[4]={ //left, right, top, bottom
  (MODULE_HALF)+0.5, -(MODULE_HALF)+0.5, MODULE_HALF, MODULE_HALF};
static int node_outputyoffset[4]={
  MODULE_HALF-OUTPUT_OFFSET, MODULE_HALF+OUTPUT_OFFSET, -MODULE_HALF, MODULE_HALF};


// module data struct
typedef struct {
  int type;
  char label[64];
  int input[4]; // input sources
  float x, y; // pos on screen
  int active; // module is active?
  int outactive; // output node active?
  int inpactive; // input node active?
  int tag; // for tagging visited modules when stacking them
  int fifopos; // stack position in the signal fifo
  int outputpos; // position of the output node
  unsigned int scale; // knob scale
  int reserved[2]; // reserved for future use
} synthmodule;


// module function call table
extern float (*mod_functable[MODTYPES])(unsigned char, float*, void*, float*);


#endif
