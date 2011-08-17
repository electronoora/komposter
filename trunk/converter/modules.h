/*
 * Komposter - module definitions and constants
 *
 * (c) 2009-2010 Firehawk/TDA
 *
 * This code is licensed under the MIT license:                               
 * http://www.opensource.org/licenses/mit-license.php  
 * 
 * Revision:    $Rev$
 * Last update: $Date$
 *
 */

#ifndef __MODULES_H__
#define __MODULES_H__

// module types defined
#define 	MODTYPES		16

// knob scales defined
#define		KNOBSCALES		9

// flags for notes
#define NOTE_LEGATO     0x0100 // tie notes together, ie. gate stays up continuously
#define NOTE_ACCENT     0x0200 // accent the note, ie. higher velocity than unaccented notes

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
#define         SCALE_RAW               0
#define         SCALE_FREQUENCY_HZ      1
#define         SCALE_FREQUENCY_TEMPO   2
#define         SCALE_DURATION          3
#define         SCALE_DURATION_TEMPO    4
#define         SCALE_RAMP              5
#define         SCALE_MIDI_NOTE         6
#define         SCALE_NOTE_INTERVAL     7
#define         SCALE_PERCENTAGE        8


// shost names for the module types
static char *modTypeNames[MODTYPES]=
{
        "KBD CV",
        "ENV",
        "VCO",
        "LFO",
        "CV knob",
        "Amp/ringmod",
        "Mixer",
        "VCF",
        "LPF",
        "Delay",
        "Scale",
        "Resample",
        "Switch",
        "Distort",
        "Accent",
        "Output"
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
	4, //switch
	2, //distort (input signal, distortion)
	0, //accent
	1  //output (input signal)
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
	2, //switch
	0, //distort
	1, //accent
	1  //output
};


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

#endif
