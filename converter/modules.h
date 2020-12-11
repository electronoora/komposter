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

#ifndef __MODULES_H__
#define __MODULES_H__

#include "constants.h"

// module types defined
#define 	MODTYPES		19

// knob scales
#define		KNOBSCALES		9

// waveforms and filter types
#define 	VCO_WAVEFORMS		4
#define 	LFO_WAVEFORMS		4
#define 	VCF_MODES		4
#define		DELAY_MODES		2
#define		SLEW_MODES		2

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
#define 	MOD_SUPERSAW		12
#define 	MOD_DISTORT		13
#define 	MOD_ACCENT		14
#define 	MOD_OUTPUT		15
#define		MOD_BITCRUSH		16
#define		MOD_SLEW		17
#define		MOD_MODULATOR		18

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
#define		SCALE_SIGNAL_AUDIO	9  // (-1, 1) domain
#define		SCALE_SIGNAL_CONTROL	10 // ( 0, 1) domain


// names for the knob scale types
#ifndef _MODULEINFO_C_
extern const char *modFunctionNames[MODTYPES];
extern const char *knobScaleNames[KNOBSCALES];
extern const char *knobScaleUnits[KNOBSCALES];
extern const char *modTypeNames[MODTYPES];
extern const char *modTypeDescriptions[MODTYPES];
extern const int modDataBufferLength[MODTYPES];
extern const int modInputCount[MODTYPES];
extern const char* modInputNames[MODTYPES][4];
extern const int modInputScale[MODTYPES][4];
extern const int modOutputScale[MODTYPES];
extern const char* modModulatorNames[MODTYPES];
extern const int modModulatorTypes[MODTYPES];
extern const char* modVcoWaveforms[VCO_WAVEFORMS];
extern const char* modLfoWaveforms[LFO_WAVEFORMS];
extern const char* modVcfModes[VCF_MODES];
extern const char* modDelayModes[DELAY_MODES];
extern const char* modSlewModes[SLEW_MODES];
extern const float node_xoffset[5][4];
extern const float node_yoffset[5][4];
extern const float node_labelpos[5][4];
extern const int node_outputCount[5];
extern const int node_outputList[5][4];
extern const int node_outputxoffset[4];
extern const int node_outputyoffset[4];
#endif

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
  unsigned char color; // signal hue; 0=inherit from upstream, 1-254=set color, 255=remove color
  unsigned char effective_color; // effective color, equals color if if nonzero, otherwise inherited
  unsigned char reserved[6];
//  int reserved[2]; // reserved for future use
} synthmodule;

#endif
