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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "audio.h"
#include "buffermm.h"
#include "modules.h"
#include "synthesizer.h"
#include "sequencer.h"

// macros for typecasting the localdata void ptr
#define mod_fdata  ((float*)data)
#define mod_fpdata ((float**)data)
#define mod_ldata  ((unsigned long*)data)
#define mod_lpdata ((unsigned long**)data)
#define mod_ddata  ((double*)data)

#define clamp(X)   fmax(fmin(X, 1.0f), 0.0f)

int noise_x1=0x67452301, noise_x2=0xefcdab89;


// jp8000 supersaw oscillator offsets and detune curve coefficents (thanks to adam szabo!)
double osc_offset[7]={0, 0.01991221, -0.01952356, 0.06216538, -0.06288439, 0.10745242, -0.11002313};
double coeftable[12]={
  0.0030115596, 0.6717417634, -24.1878824391, 404.2703938388, -3425.0836591318, 17019.9518580080, -53046.9642751875,
  106649.6679158292, -138150.6761080548, 111363.4808729368, -50818.8652045924, 10028.7312891634};

// tables for detune and mix coefficents for modulator values 0..127
float supersaw_detune[128][7];
float supersaw_mix[128][7];


float pitch[MAX_SYNTH];
float accent[MAX_SYNTH];


// these are just 1-bit flags
int gate[MAX_SYNTH];

// this has flags for different restart types
int restart[MAX_SYNTH];




////////////////////////////////////////////////
//
// module functions
//
///////////////////////////////////////////////


MODULE_FUNC(kbd) { return *mod=pitch[v]/OUTPUTFREQ; }


MODULE_FUNC(output) { return ms[0]*(*mod); }


MODULE_FUNC(accent) {
  return accent[v] ? *mod : 0.0;
}


MODULE_FUNC(vco) // phase-accumulating oscillator w/ suboscillator
{
  float out;
  
  mod_fdata[0]+=ms[0];
  mod_fdata[0]-=floor(mod_fdata[0]);

  // advance subosc
  mod_fdata[1]+=ms[0]/2;
  mod_fdata[1]-=floor(mod_fdata[1]);

  // hard restart
  if (restart[v]&SEQ_RESTART_VCO) { mod_fdata[0]=0; mod_fdata[1]=0; }

  switch((int)(*mod))
  {
    case VCO_PULSE:    out=(mod_fdata[0] < ms[1]) ? -1.0 : 1.0; break;
    case VCO_SAW:      out=(mod_fdata[0] * 2 - 1.0f); break;
    case VCO_TRIANGLE: out=(mod_fdata[0]<0.75) ? 1-fabs(mod_fdata[0]*4-1) : 1-fabs(mod_fdata[0]*4-5); break;
    case VCO_SINE:     out=sin(2*3.1415926* mod_fdata[0]); break;
    break;
  }

  // suboscillator (pulse at -1 octave)
  out+=ms[2]*((ms[1]<mod_fdata[1])?-1.0:1.0);

  // noise
  noise_x1^=noise_x2;
  out+=ms[3]*(noise_x2*(2.0f/0xffffffff));
  noise_x2+=noise_x1;
  
  return out;
}


MODULE_FUNC(lfo) { // low-frequency oscillator, input is freq in hz, cv output (0 to 1.0)
  float out;

  // hard restart
  if (restart[v]&SEQ_RESTART_LFO) { mod_fdata[0]=0; mod_fdata[1]=0; }
  
  mod_fdata[0]+=ms[0];
  mod_fdata[0]-=floor(mod_fdata[0]);

  switch((int)(*mod)) {
    case LFO_TRIANGLE: out=2*mod_fdata[0]; if (out>1.0) out=2-out; break;
    case LFO_SINE:     out=-0.5*(cos(2*3.1415926*mod_fdata[0])-1); break;
  }
  out*=ms[1];
  out+=ms[2];
  
  return out;
}


MODULE_FUNC(env) // linear adsr envelope generator
{
  int i;

  // hard restart
  if (restart[v]&SEQ_RESTART_ENV) { mod_fdata[0]=0; mod_ldata[1]=0; }
  
  if (gate[v]) {
    if (!mod_ldata[1]) mod_ldata[2]=1; // trig if gate went 0->1
  if (mod_ldata[2]) {
      mod_fdata[0]+=ms[0]; if (mod_fdata[0]>=1.0) {
       mod_ldata[2]=0;
       mod_fdata[0]=1.0; } // attack
    } else {
      mod_fdata[0]-=ms[1]; if (mod_fdata[0]<ms[2]) mod_fdata[0]=ms[2]; // decay+sustain
    }
  } else {
    mod_fdata[0]-=ms[3]; if (mod_fdata[0]<0.0) mod_fdata[0]=0.0; // release
  }
  mod_ldata[1]=gate[v]; // save current gate
  return mod_fdata[0];
}


MODULE_FUNC(vcf) // 12db/oct resonant state variable low-/high-/bandpass filter
{
  float f, q, r, out;
  // in1=signal in, in2=cutoff 0.0~1.0 (=0-fs), in3=resonance 0.0~1.0

  // safety nets to keep the filter from going nuts
  if (ms[1]>1.0) ms[1]=1.0;
  if (ms[1]<0.0) ms[1]=0.0;
  if (ms[2]>1.0) ms[2]=1.0;
  if (ms[2]<0.0) ms[2]=0.0;

  // float *data -> 0=lpf, 1=hpf, 2=bpf
  f = 2*sin(3.14159 * ms[1]); // cutoff in [0.0, 1.0]
  q=1.0-ms[2];
  r=sqrt(q);
  mod_fdata[0] = mod_fdata[0] + f * mod_fdata[2];
  mod_fdata[1] = r * ms[0] - mod_fdata[0] - q * mod_fdata[2];
  mod_fdata[2] = f * mod_fdata[1] + mod_fdata[2];

  // generate filter output
  out=0.0;
  switch((int)(*mod)) {
    case VCF_OFF:      out=ms[0]; break;
    case VCF_LOWPASS:  out+=mod_fdata[0]; break;
    case VCF_HIGHPASS: out+=mod_fdata[1]; break;
    case VCF_BANDPASS: out+=mod_fdata[2]; break;
  }
  
  return out;
}




MODULE_FUNC(delay)
{
  float *buffer, out, spfrac;
  long writeptr, readptr, loopend, ptrdelta;

  buffer=mod_fpdata[0]; // data[0] is a ptr to a float buffer
  writeptr=mod_ldata[1];

  if (!buffer) return ms[0]; // failsafe - return the input if no buffer

  // delay and loop in samples
  loopend=3*OUTPUTFREQ; // 3sec maximum
  if (ms[2]>1) loopend=ms[2]; // use loop input if greater than 1 sample
  ptrdelta=(long)(ms[1]); // truncate fractional part
  spfrac=ms[1]-(float)(ptrdelta);

  readptr=(writeptr - ptrdelta);
  while (readptr<0) readptr+=loopend;
  out=buffer[readptr]*spfrac;
  readptr++; readptr%=loopend;
  out+= buffer[readptr]*(1-spfrac);

  if ((int)(*mod)==DELAY_ALLPASS) out+=ms[0]*(-ms[3]); // feedforward for allpass
  buffer[writeptr]=ms[0] + out*ms[3]; 

  mod_ldata[1]=(writeptr+1)%loopend;

  return out;
}


MODULE_FUNC(dist)  { // simple clipping distort

  float out;
  out=ms[0];
  out*=ms[1]; // ampl
  if (fabs(out)>1.0) out = out/fabs(out);
  return out;

/*
  float a,k,x,y;
  a=clamp(ms[1]);
  if (a>=1.0f) a=0.9999f;
  
  k=2*a/(1-a);
  x=ms[0];
  y=(1+k)*x/(1+k*abs(x));
  return y;
*/


/*
  float y,x;
  y=ms[0]*ms[1];
  x=fabs(y);
  if (x>1.0) x=(2.0 - 1.0/x);
  x=(y/fabs(y));
  return y;
*/
}


MODULE_FUNC(resample) { // sample-and-hold
  // ms[0] is input signal
  // ms[1] is sample rate as accumulator delta

  mod_fdata[0]-=ms[1];
  if (mod_fdata[0]<0) {
    mod_fdata[1]=ms[0]; // sample from input
    mod_fdata[0]=1.0f; // reset accumulator
  }
  return mod_fdata[1];
}


// simple basic operators
MODULE_FUNC(cv) { return *mod; }
MODULE_FUNC(amp) { return ms[0]*ms[1]; }
MODULE_FUNC(att) { return ms[0]* *mod; }
MODULE_FUNC(mixer) { return ms[0]+ms[1]+ms[2]+ms[3]; }



MODULE_FUNC(lpf24) { // 24db/oct four-pole low pass
  // ms[0]=signal in, ms[1]=cutoff (0..1), ms[2]=resonance (0..1)
  
  // safety nets to keep the filter from going nuts
  if (ms[1]>1.0) ms[1]=1.0;
  if (ms[1]<0.0) ms[1]=0.0;
  if (ms[2]>1.0) ms[2]=1.0;
  if (ms[2]<0.0) ms[2]=0.0;

  double f = ms[1]*1.16*3;
  double fb = (ms[2]*4.0) * (1.0 - 0.15 * f * f);
  double input = ms[0] - mod_ddata[3] * fb;
  input *= 0.35013 * (f*f)*(f*f);

  mod_ddata[0] = input        + 0.3 * mod_ddata[4] + (1 - f) * mod_ddata[0]; // Pole 1
  mod_ddata[4] = input;
  mod_ddata[1] = mod_ddata[0] + 0.3 * mod_ddata[5] + (1 - f) * mod_ddata[1]; // Pole 2
  mod_ddata[5] = mod_ddata[0];
  mod_ddata[2] = mod_ddata[1] + 0.3 * mod_ddata[6] + (1 - f) * mod_ddata[2]; // Pole 3
  mod_ddata[6] = mod_ddata[1];
  mod_ddata[3] = mod_ddata[2] + 0.3 * mod_ddata[7] + (1 - f) * mod_ddata[3]; // Pole 4
  mod_ddata[7] = mod_ddata[2];
  
  return mod_ddata[3]; //out4;
}


/* waveshaper
float waveshape_distort( float in ) {
  return 1.5f * in - 0.5f * in *in * in;
  }
*/


/*
MODULE_FUNC(comp)
{
}


MODULE_FUNC(envdet) { // envelope follower: ms[0] = input, ms[1] = attack, ms[2] = release
//  float attack_coef = exp(log(0.01)/( ms[1] * OUTPUTFREQ * 0.001));
//  float release_coef = exp(log(0.01)/( ms[2] * OUTPUTFREQ * 0.001));

  // attack and release inputs are in duration (sec) scale
  float attack_coef = exp(log(0.01)/ms[1]);
  float release_coef = exp(log(0.01)/ms[2]);

  float tmp = fabs(ms[0]);
  if(tmp > mod_fdata[0])
    mod_fdata[0] = attack_coef * (mod_fdata[0] - tmp) + tmp;
  else
    mod_fdata[0] = release_coef * (mod_fdata[0] - tmp) + tmp;

  return mod_fdata[0];
}
*/


float sawtooth(float ac) {
//  return -1.0 + 2.0*ac;
  return -1.0 + 2.0*sqrt(ac);
}

MODULE_FUNC(supersaw) {
  float f, q, r;
  float out=0;
  int i;
  float m_pitch;
  int m_mix, m_detune;

  m_pitch=ms[0];
  m_detune=(127 * clamp(ms[1]));
  m_mix=(127 * clamp(ms[2]));

  // generate waveform and step accumulators
  out=0;
  for(i=0;i<7;i++) {
    out+=sawtooth(mod_fdata[i])*supersaw_mix[m_mix][i];
    mod_fdata[i]+=supersaw_detune[m_detune][i]*m_pitch;
    mod_fdata[i]-=floor(mod_fdata[i]);
  }

  // highpass
  f = 2*sin(3.14159 * m_pitch); // cutoff in [0.0, 1.0]
  q=1.0 - 0.2; // resonance is 0.2
  r=sqrt(q);
  mod_fdata[8] = mod_fdata[8] + f * mod_fdata[10];
  mod_fdata[9] = r * out - mod_fdata[8] - q * mod_fdata[10];
  mod_fdata[10] = f * mod_fdata[9] + mod_fdata[10];
  out = mod_fdata[9];
  
  return out;
}




// module function call table
float (*mod_functable[MODTYPES])(unsigned char, float*, void*, float*)={
		modfunc_kbd,
		modfunc_env,
		modfunc_vco,
		modfunc_lfo,
		modfunc_cv,
		modfunc_amp,
		modfunc_mixer,
		modfunc_vcf,
		modfunc_lpf24,
		modfunc_delay,
		modfunc_att,
		modfunc_resample,
		modfunc_supersaw,
		modfunc_dist,
		modfunc_accent,
		modfunc_output
};

