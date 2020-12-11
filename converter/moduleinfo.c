/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Implementations for the synthesizer modules
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define _MODULEINFO_C_
#include "modules.h"

////////////////////////////////////////////////
//
// module definitions, copied from main app modules.c
//
///////////////////////////////////////////////


// function name suffixes all module types
const char *modFunctionNames[MODTYPES]=
{
        "kbd",
        "env",
        "vco",
        "lfo",
        "accent",
        "amp",
        "mixer",
        "vcf",
        "lpf24",
        "delay",
        "att",
        "resample",
        "supersaw",
        "dist",
        "accent",
        "output",
        "bitcrush",
        "slew",
        "cv"
};


// names for the knob scale types
const char *knobScaleNames[KNOBSCALES]=
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
const char *knobScaleUnits[KNOBSCALES]=
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
const char *modTypeNames[MODTYPES]=
{
	"KBD CV",
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
	"Suprsaw", //Switch",
	"Distort",
	"Accent",
	"Output",
	"Bitcrush",
	"Slew",
	"Mod CV"
};

// Module descriptions
const char *modTypeDescriptions[MODTYPES]=
{
	"Keyboard control voltage (CV) from the sequencer for setting the VCO frequency. The voltage\nis 1 unit/hz, so an A-3 generates a voltage of 440.",
	"ADSR envelope generator with linear attack and decay/release ramps.\nSustain level sets the amplification, A/D/R ramps are set as a duration",
	"Voltage controlled oscillator (VCO). Outputs the chosen waveform at a\nfrequency set by the control voltage. Includes a noise generator\nand -1 octave pulse suboscillator with levels\nselectable using control inputs.",
	"The low frequency oscillator (LFO) can be used to, for example, make\nlong filter- and pulse width sweeps, vibrato or other effects\nlike chorus and flanging.",
	"Knobs output a control voltage which can be used to control the\nother modules. There is no forced voltage range but 0 to 1 is normally used.",
	"Amplitude modulation amplifies or attenuates the input signal\naccording to the control voltage. Since this is done by multiplying,\nit also works as a ring modulator.",
	"Mixes up to four signals together by adding. Works also an an\namplifier or a frequency multiplier by connecting the same signal to\nmultiple inputs",
	"State-variable voltage controlled filter (VCF). Resonant low-,\nband- and highpass filter with 12db/oct attenuation.",
	"Voltage controlled 24db/oct 4-pole resonant lowpass filter (LPF).\nSelf-oscillates when resonance is set to 100%.",
	"Delay module which can operate either as a comb filter or an\nallpass filter. Has control inputs for gain/feedback, loop length and\ndelay time. When feedback is not connected or zero, it functions\nas a normal one-tap variable delay. The loop length is 3 seconds when\nno control input is provided.",
	"Scaler knobs can be used attenuate or amplify an incoming signal\nby multiplying it with the modulator value.",
        "Sample-and-hold resamples the signal down by sampling it at given\nintervals and outputting the sampled value until a new one is\nread. The sample rate input is a frequency in hz",
        "Seven-oscillator supersaw generator with detune and mix controls.\nEmulates closely the output of a Roland JP8000/JP8080 supersaw.",
	"Amplifies and distorts the input signal by a variable amount",
	"The accent modules outputs the user-specified voltage when a sequencer note\nbeing played has the accent mark. It can be used to change the volume, resonance or any other feature of the note.",
	"Outputs the signal to the master mixer at the selected output level",
	"Bitcrusher restricts the sample resolution by a variable amount",
	"Slew limiter is a specialized low-pass filter typically used for\nglide/portamento",
	"Modulator control voltage (CV) allows you to tap KBD CV from\nother channels to use for modulation"
};


// number of dwords a module requires for buffer. ptr goes always to
// first dword of the localdata
const int modDataBufferLength[MODTYPES]={
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
	0, //output
	0, //bitcrush
        0, //slew
        0  //modulator
};


// Number of input nodes on modules
const int modInputCount[MODTYPES]={
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
	1, //output (input signal)
	2, //bitcrush (input signal, amount)
	2, //slew (input, amount)
	0  //modulator
};


// Labels for all input nodes on modules
const char* modInputNames[MODTYPES][4]={
{"","","",""},
{"A", "d", "s", "r"},
{"frq", "pwm", "sub", "nse"},
{"frq", "AmpL", "bias", ""},
{"","","",""},
{"in", "Amp", "", ""},
{"in1", "in2", "in3", "in4"},
{"in", "fc", "rEs", ""},
{"in", "fc", "rEs", ""},
{"in", "timE", "loop", "fb"},
{"in", "", "", ""},
{"in", "rate", "", ""},
{"frq", "dtune", "mix", ""},
{"in", "dist", "", ""},
{"","","",""},
{"in", "", "", ""},
{"in", "dep", "", ""},
{"in", "amt", "", ""},
{"", "", "", ""}
};


// expected scale of each input on the modules
const int modInputScale[MODTYPES][4]={
{0, 0, 0, 0},
{SCALE_RAMP, SCALE_RAMP, SCALE_PERCENTAGE, SCALE_RAMP},
{SCALE_FREQUENCY_HZ, SCALE_PERCENTAGE, SCALE_PERCENTAGE, SCALE_PERCENTAGE},
{SCALE_FREQUENCY_HZ, 0, 0, 0},
{0, 0, 0, 0},
{0, 0, 0, 0},
{0, 0, 0, 0},
{0, SCALE_FREQUENCY_HZ, SCALE_PERCENTAGE, 0},
{0, SCALE_FREQUENCY_HZ, SCALE_PERCENTAGE, 0},
{0, SCALE_DURATION, SCALE_DURATION, SCALE_PERCENTAGE},
{0, 0, 0, 0},
{0, SCALE_FREQUENCY_HZ, 0, 0},
{SCALE_FREQUENCY_HZ, SCALE_PERCENTAGE, SCALE_PERCENTAGE, 0},
{0, SCALE_PERCENTAGE, 0, 0},
{0, 0, 0, 0},
{0, 0, 0, 0},
{0, SCALE_PERCENTAGE, 0, 0},
{0, SCALE_PERCENTAGE, 0, 0},
{0, 0, 0, 0}
};


// output signal type of each module type, 0 if use is varying
const int modOutputScale[MODTYPES]={
  SCALE_FREQUENCY_HZ,
  SCALE_PERCENTAGE,
  SCALE_SIGNAL_AUDIO,
  SCALE_PERCENTAGE,
  0, // knob
  0, // amp
  0, // mixer
  SCALE_SIGNAL_AUDIO,
  SCALE_SIGNAL_AUDIO,
  SCALE_SIGNAL_AUDIO,
  0,
  SCALE_SIGNAL_AUDIO,
  SCALE_SIGNAL_AUDIO,
  SCALE_SIGNAL_AUDIO,
  SCALE_PERCENTAGE,
  0,
  0,
  0,
  SCALE_FREQUENCY_HZ
};


// modulator names for each module
const char* modModulatorNames[MODTYPES]={
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
"level",
"",
"lin/log",
"channel"
};

// modulator value type. 0=no modulator, 1=float, 2=integer
const int modModulatorTypes[MODTYPES]={
	1, //CV
	0, //ADSR
	3, //wave
	4, //lfo
	1, //knob
	0, //amp
	0, //mixer
	5, //filter
	0, //lpf24
	6, //delay
	1, //attenuator
	0, //resample
	0, //supersaw
	0, //distort
	1, //accent
	1, //output
	0, // bitcrush
        7, // slew
        8  // modulator
};



 char* modVcoWaveforms[VCO_WAVEFORMS]={"Pulse", "Saw", "Triangle", "Sine"}; //, "Noise"};
 char* modLfoWaveforms[LFO_WAVEFORMS]={"Square", "Saw", "Triangle", "Sine"};
 char* modVcfModes[VCF_MODES]={"Off", "Lowpass", "Highpass", "Bandpass"};
 char* modDelayModes[DELAY_MODES]={"Comb filter", "Allpass filter"};
 char* modSlewModes[SLEW_MODES]={"Linear", "Logarithmic"};


