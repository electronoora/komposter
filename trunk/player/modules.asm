;;
;; Komposter player (c) 2010 Firehawk/TDA
;;
;; This code is licensed under the MIT license:                             
;; http://www.opensource.org/licenses/mit-license.php
;;
;; This file contains all the implementations for the modules
;; used for sound synthesis. Note that some of the modules are
;; not fully optimized for size yet.
;;

;
; the synthesizer modules are always called with the registers set up as
; follows:
;
; flags      = cl
; voice	     = edx
; *data      = esi
; mod	     = eax (as integer)
;            = st4 (as single float)
; ms[0]      = st0
; ms[1]	     = st1	
; ms[2]      = st2
; ms[3]      = st3
;
; the module returns its output in st0.
;




; vco waveforms
%define         VCO_PULSE               0
%define         VCO_SAW                 1
%define         VCO_TRIANGLE            2
%define         VCO_SINE                3



; this is used to hard restart the accumulators on ADSR, VCO and LFO
; old flags in env are also zeroed on hard restart
hardrestart:
	test	cl, 1 		; module shifts its restart flag to lsb
	jz	.restart_out
	xor	ebx, ebx
	mov	[esi], ebx  	; clear accumulator
	mov	[esi+4], ebx	; clear subosc accumulator for vco
.restart_out:
	ret



; MODULE_FUNC(kbd)
module_func_kbd:
	fld	dword SONGBSS(pitch+edx*4) ; only place where channel nbr in edx is used
	ret



; MODULE_FUNC(env)
module_func_env:
	; st0=attack, st1=decay, st2=sustain, st3=release, st4=0
	; [esi] = accumulator, byte [esi+4]=old flags w/ trig
	push	cx
	shr	cl, 4
	call 	hardrestart
	pop	ax
.env_process:
	fld	dword [esi]	;accumulator to st0
	; st0=acc, st1=att, st2=dec, st3=sus, st4=rel, st5=0
	test	al, FLAG_GATE
	jz	.env_release
	; gate is up, was it down previously?
	mov 	cl, [esi+4]
	test 	cl, FLAG_GATE
	jnz 	.env_notrig;
	or 	al, FLAG_TRIG
.env_notrig:
	and 	cl, FLAG_TRIG
	or 	al, cl
	test	al, FLAG_TRIG
	jz	.env_decay_sustain
	fadd	st0, st1	;add attack rate to acc
	fld1
	fcomip	st1
	jnc	.env_done 	; return current acc
	fld1			; return 1.0 in st0
	and	al, 0xff-FLAG_TRIG	; trig off
	jmp	.env_done
.env_decay_sustain:
	fsub	st0, st2
	fcomi	st3		; compare to sustain
	jnc	.env_done
	fxch	st3		; return sustain level
	jmp	.env_done
.env_release:
	fsub	st0, st4	; dec by release
	fldz
	fcomip	st1
	jc	.env_done	; return current acc
	fldz			; return zero as acc was < 0
.env_done:
	fst	dword [esi]	;store new acc back and return as output
	mov	[esi+4], al ; store old flags and trig
	ret



; MODULE_FUNC(vco) {
module_func_vco: ; phase-accumulating oscillator w/ suboscillator
	shr	cl, 5
	call	hardrestart

	fld1				; 1.0 cv  pwm sub nse jnk
	fld	st1			; cv  1.0 cv  pwm sub nse jnk
	fadd	dword [esi]		; acc 1.0 cv  pwm sub nse jnk
	fprem				; acc 1.0 cv  pwm sub nse jnk
	fst	dword [esi]		;;; store main oscillator accumulator
	fstp	st6			; 1.0 cv  pwm sub nse acc
	fxch	st0, st1		; cv  1.0 pwm sub nse acc
	fmul	dword SONGDATA(half)    ; scv 1.0 pwm sub nse acc
	fadd	dword [esi+4]		; sac 1.0 pwm sub nse acc
	fprem				; sac 1.0 pwm sub nse acc
	fstp	dword [esi+4]		;;; store subosc accumulator
	fcomp	st0, st1		; pwm sub nse acc

	; modulator is already in eax
        ; fpu: pwm, sub, noise, acc
	cmp	al, VCO_PULSE
	jnz	.vco_saw
.vco_pulse:
	fcomi	st0, st3		; compare pwm vs accumulator
	fld1
	jc	.vco_oscdone
	fchs
.vco_saw:
	cmp	al, VCO_SAW
	jnz	.vco_triangle
	fld1
	fld	st4
	fadd	st0, st0
	fsubrp	st1, st0
.vco_triangle:
	cmp	al, VCO_TRIANGLE
	jnz	.vco_sine
	fld	st3
	fld	dword SONGDATA(threefourths)
	fcomip	st0, st1
	fmul	dword SONGDATA(four)	;4*acc
	jc	.vco_trilastq
	fld1
	jmp	.vco_tricalc
.vco_trilastq:
	fld	dword SONGDATA(five)
.vco_tricalc:
	fsubp	st1, st0	;acc*4 - [1 or 5]
	fabs
	fld1
	fsubrp	st1, st0	;1-fabs(acc*4 - [1 or 5])
.vco_sine:
	cmp	al, VCO_SINE
	jnz	.vco_oscdone
	; none of the others matched, so it has to be sine
	fldpi
	fadd	st0, st0
	fmul	st4
	fsin
.vco_oscdone:
	; osc pwm sub nse acc

.vco_sub:
	fld	dword [esi+4]	; sac osc pwm sub nse acc
	fcomip	st0, st2	; osc pwm sub nse acc
	fxch	st0, st2	; sub pwm osc nse acc
	jc	.vco_suboscdone	; jump depending on the fcomip
	fchs
.vco_suboscdone:
	; sub  pwm  osc  noise  acc
	faddp	st2, st0	; pwm out noise acc

	; noise
	;out=noise*(noise_x2*(2.0f/0xffffffff));
	fxch	st0, st2 ; noise  osc+sub  pwm  acc
	fadd	st0, st0 ; 2*noise  osc+sub  pwm  acc
	fild	dword SONGBSS(noise_x2)
	fmulp	st1, st0 ; 2*noise*x2   osc+sub  pwm acc
	fdiv 	dword SONGDATA(noise_div)
	faddp	st1, st0

	; done, st0 has output
	ret



; MODULE_FUNC(lfo) { // low-frequency oscillator
module_func_lfo:
	; st0=freq, st1=ampl, st2=bias, st3=0, st4=junk
	shr	cl, 6
	call	hardrestart
.lfo_osc:
	fadd	dword [esi]	; accu ampl bias 0.0  junk
	fld1			; 1.0  accu ampl bias 0.0  junk
	fxch 	st0, st1	; accu 1.0  ampl bias 0.0  junk
	fprem			; accu 1.0 ...
	fst	dword [esi]	
				;acc,  1.0,  ampl,  bias,  0,  junk
.lfo_triangle:
	and	al, al
	jz	.lfo_sine
	fadd	st0, st0	;2*acc	
	fcomi	st0, st1	;2acc  1.0  ampl  bias  0.0  junk
	jc	.lfo_done
	fchs			;-2acc ..
	fadd	st0, st1	;-2acc+1
	fadd	st0, st1	;-2acc+2
	jmp	.lfo_done
.lfo_sine:
 	fldpi
	fadd	st0, st0
	fmulp	st1, st0
	fcos
	fsub	st0, st1
	fmul 	dword SONGDATA(half)
	fchs
.lfo_done:
	; fpu should now be:
	; out,  1.0,  ampl,  bias,  0,  junk
	fmul	st0, st2
	fadd	st0, st3
	ret



; MODULE_FUNC(accent) { return accent[v] ? *mod : 0.0; }
; MODULE_FUNC(cv) { return *mod; }
; MODULE_FUNC(amp) { return ms[0]*ms[1]; }
module_func_accent:
	; st0 to st3 are zero
	test	cl, FLAG_ACCENT
	jnz	module_func_cv
module_func_amp:
	fmul	st1	; return 0*0 on accent, amp*input on amp
	ret
module_func_cv:
	fxch	st4	; return modulator in st0
	ret



; MODULE_FUNC(output) { return ms[0]*(*mod); }
; MODULE_FUNC(att) { return ms[0]*(*mod); }
module_func_output:
module_func_att:
	fmul	st4		; ms0 * mod
	ret



; MODULE_FUNC(mixer) { return ms[0]+ms[1]+ms[2]+ms[3]; }
module_func_mixer:
        ; s0 s1 s2 s3 mod
	faddp	st1  ; s0+s1  s2  s3  mod
	faddp	st1  ; s0+s1+s2  s3  mod
	faddp	st1  ; s0+s1+s2+s3  mod
	ret



; MODULE_FUNC(vcf) // 12db/oct resonant state variable low-/high-/bandpass filter
module_func_vcf:
	; esi = lp, esi+8 = bp, esi+4 = hp
	and	al, 3
	jz	.vcf_out	; filter is off, return unmodified input
	dec	al 		; 0=lp, 1=hp, 2=bp
	shl	al, 2

	; q = 1.0 - q  -- no resonance when res input is not connected
	fld1
	fsub	st0, st3
	fstp	st3
				; s      fc     q      0      0
	fstp	st3		; fc     q      s      0 
	fldpi            	; pi     fc     q      s      0
	fmulp 	st1        	; pi*fc  q      s      0
	fsin             	; sin(pi*fc) q  s      0
	fadd 	st0, st0    	; f      q      s      0
	fld	st0		; f      f      q      s      0
	fmul	dword [esi+8]	; f*bp   f      q      s      0
	fadd	dword [esi]	; lp+f*bp  f    q      s      0
	fst	dword [esi]	; lp     f      q      s      0
	fld	st2		; q      lp     f      q      s      0
	fsqrt			; r      lp     f      q      s      0
	fmul	st0, st4	; r*s    lp     f      q      s      0
	fsubrp	st1, st0	; r*s-lp f      q      s      0
	fld	st2		; q      r*s-lp f      q      s      0
	fmul	dword [esi+8]	; q*bp   r*s-lp f      q      s      0
	fsubp	st1, st0	; r*s-lp-q*bp, f, q    s      0
	fst	dword [esi+4]	; hp     f      q      s      0
	fmulp	st1, st0	; f*hp   q      s      0
	fadd	dword [esi+8]	; bp+f*hp q     s      0
	fstp	dword [esi+8]	; q      s      0
	fld 	dword [esi+eax] ; out    q      s      0
.vcf_out:
	ret



;MODULE_FUNC(lpf24) { // 24db/oct four-pole low pass
module_func_lpf24:
        ;   in      fc     q      0      0
        fxch    st0, st1
        fmul    dword [three_point_foureight] ; f=fc*3.48
        fld     st0  ;  f,   f, in, q, 0, 0
        fmul    st1  ;  f^2, f, ..
        fld     st0  ;  f^2, f^2, f, ...
        fmul    st1  ;  f^4, f^2, f, in, q, 0, 0
        fmul    dword [lpf_feedback_coef]
        fxch    st0, st1        ; f^2, 0.35013*f^4, f, in, q, 0, 0
        fmul    dword [minus_point_onefive]
        fld1
        faddp   st1, st0 ; 1.0-0.15*f^2, 0.3504*f^4, f, in, q, 0, 0
        fld     st4 ; q, ...
        fadd    st0 ; q*2, ...
        fadd    st0 ; q*4, 1.0-...
        fmulp   st1, st0 ; fb, 0.3504*f^4, f, in, q, 0, 0
        fmul    qword [esi+4*8]
        fchs
        fadd    st3
        fmulp   st1, st0
        fstp    qword [esi] ; feed pole 1
        ;f, in, q, 0, 0

        ; calculate all four poles
        ; p_inputs from esi+0, p_outputs from esi+8, p_states from esi+40
        xor     ecx, ecx
        mov     cl, 4
.lpf24_pole:
        ; st0 = f, esi = base offset for pole input
        fld1
        fsub    st0, st1       ; 1-f, f, in, q, 0, 0
        fmul    qword [esi+8]  ; (1-f)*p_out, f, in, q, 0, 0
        fld     dword [point_three]
        fmul    qword [esi+40] ; 0.3*p_state, (1-f)*p_out, f, in, q, 0, 0
        faddp   st1, st0       ; 0.3*p_state + (1-f)*p_out, f, in, q, 0, 0
        fld     qword [esi]    ; p_input, 0.3*p_state + (1-f)*p_out, f, in,q, 0, 0
        fst     qword [esi+40]
        faddp   st1, st0       ; p_input + 0.3*p_state + (1-f)*p_out, f, in, q, 0, 0
        fstp    qword [esi+8]
        add     esi, 8
        loop    .lpf24_pole
        fld     qword [esi] ; return pole 4 output
        ret




;MODULE_FUNC(delay) // interpolated comb/allpass filter delay
module_func_delay:
	; st0=in, st1=time, st2=loop, st3=fb
	push	eax
	mov	edi, [esi]
	cmp	edi, 0	; do we have a buffer already
	jnz	.delay_bufok
	movzx	eax, byte SONGBSS(delaycount)
	shl	eax, 18 ; = imul eax, eax, DELAYBUFFERSIZE
	lea	edi, SONGBSS(delaybuffer+eax*4)
	inc	byte SONGBSS(delaycount)
	mov	[esi], edi
.delay_bufok:
	mov	ebx, [esi+4]
	; edi is ptr to delay buffer
	; ebx is write offset

	mov	ecx, DELAYBUFFERSIZE	
;; ignoring the loop input for now
;	sub	esp, 4
;	fld	st2
;	fistp	dword [esp] ; loopend in samples
;	pop	ecx

;ret

	;  ptrdelta=(long)(ms[1]); // truncate fractional part
	;  spfrac=ms[1]-(float)(ptrdelta);
	sub	esp, 4
	fld	st1		; time  in   time  loop  fb
	fistp	dword [esp]
	fild	dword [esp]	; trunc(time)   in   time   loop  fb
	pop	edx ; ptrdelta
	fsubr	st0, st2	; spfrac  in  time  loop  fb

	;  readptr=(writeptr - ptrdelta);
	;  while (readptr<0) readptr+=loopend;
	mov	eax, ebx
	sub	eax, edx	; eax = readptr
	jns	.delay_inrange
	add	eax, ecx
.delay_inrange:

	; interpolation
	;  out=buffer[readptr]*spfrac;
	;  readptr++;
	;  readptr%=loopend;
	;  out+= buffer[readptr]*(1-spfrac);
	fld	dword [edi+eax*4]
	fmul	st0, st1	; out  spfrac  in  time  loop  fb
	inc	eax
	xor	edx, edx
	idiv	ecx		; edx = remainder from readptr/loopend
	fld1			;  1  out1  spfrac  in  time  loop  fb
	fsub	st0, st2	; 1-spfrac  out1  spfrac  in  time  loop  fb
	fmul	dword [edi+edx*4] ; out2  out1  spfrac  in  time  loop  fb
	faddp	st1, st0	; out  spfrac  in  time  loop  fb

	pop	eax
	and	al, al
	jz	.delay_comb 
	; allpass mode, do a feedforward
	; out+=ms[0]*(-ms[3]);
	fld	st2		; in  out  spfrac  in  time  loop  fb
	fmul	st6             ;in*fb ...
	fchs
	faddp	st1, st0
.delay_comb:

	;  buffer[writeptr]=ms[0] + out*ms[3]; 
	fld	st0		; out  out  spfrac  in  time  loop fb
	fmul	st0, st6	; out*fb  out  spfrac  in  time  loop  fb
	fadd	st0, st3	; in+out*fb  out  spfrac  in  time  loop  fb
	fstp	dword [edi+ebx*4]

	;  mod_ldata[1]=(writeptr+1)%loopend;
	inc	ebx
	mov	eax, ebx
	xor	edx, edx
	idiv	ecx
	mov	[esi+4], edx

	ret





; MODULD_FUNC(supersaw)  {  // jp8000-like detunable 7-sawtooth VCO
module_func_supersaw:
	; not yet implemented. requires some setup prior to playback.
	ret



module_func_resample:
	; st0=in, st1=rate, st2=0. st3=0, st4=0
	fxch	st0, st1	;rate, in, 0, 0, 0
	fld	dword [esi]	;acc, rate, in, 0, 0, 0
	fsubrp	st1, st0	;acc-rate, in, 0, 0, 0
	fst	dword [esi]
	fldz
	fcomip	st0, st1
	jc	.snh_out
	fxch	st0, st1	; in, acc-rate, 0, 0, 0
	fstp	dword [esi+4]	; sample the input
	fld1
	fstp	dword [esi]	; reset acc back to 1.0
.snh_out:
	fld	dword [esi+4]	; return held sample
	ret



; MODULE_FUNC(dist)  { // simple clipping distort, input 1 is amplification
module_func_dist:
	; in  amp  0.0  0.0  0.0
	fmul	st0, st1 ; in*amp  amp
	fld	st0	; out  out  amp
	fabs		; abs(out)  out  amp
	fxch	st0, st1
	fld1		; 1.0  out  abs(out)
	fcomip	st0, st2	;out  abs(out)
	jnc	.dist_noclip
	fdiv	st0, st1
.dist_noclip:
	ret


