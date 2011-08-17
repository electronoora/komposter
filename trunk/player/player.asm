;;
;; Komposter player (c) 2011 Firehawk/TDA
;;
;; This code is licensed under the MIT license:                             
;; http://www.opensource.org/licenses/mit-license.php
;;
;; This is a reference implementation for a Komposter playroutine
;; in x86 assembly language.
;;

; macros SONGDATA(address) and SONGBSS(address) are used for
; getting a pointer address to the data. change these macros
; if you relocate them
;todo: remove, not needed with laturi
%define	SONGDATA(X) [X]
%define SONGBSS(X)  [X]

; output sample rate
%define 	OUTPUTFREQ	44100

; how many samples of audio we're rendering
%define		SONG_BUFFERLEN	75*OUTPUTFREQ

; maximum number of modules per synth. 64 means 256 bytes of modulator data per channel.
%define		MAX_MODULES		64

; delay buffer addresses are max. 18bit offsets
%define 	DELAYBUFFERSIZE		262144


; flags in voice data byte
%define		FLAG_GATE		1
%define		FLAG_TRIG		2
%define		FLAG_ACCENT		4
%define		FLAG_NOTEOFF		8
%define		FLAG_RESTART_ENV	16
%define		FLAG_RESTART_VCO	32
%define		FLAG_RESTART_LFO	64
%define		FLAG_LOAD_PATCH		128



;
; DATA
;
section .data

outputfreq	dd	44100.0  ; outputfreq as a float value
noise_x1	dd	0x67452301
noise_x2	dd	0xefcdab89
noise_div	dd	4294967296.0
midi0           dd      8.1757989156 ; C0
midisemi        dd      1.059463094 ; ratio between notes a semitone apart

; small constants used around the code
half		dd	0.5
threefourths	dd	0.75
four		dd	4.0
five		dd	5.0

; these are for the 24db/oct lpf, comment out if not used
three_point_foureight   dd      3.48
minus_point_onefive     dd      -0.15
lpf_feedback_coef       dd      0.35013  
point_three             dd      0.3



; jump table to module functions. if you know that your song doesn't use
; some module, delete the function from modules.asm and set the jump table
; address here to zero. probably saves a few bytes.
modfunctable:
	dd	module_func_kbd
	dd	module_func_env
	dd	module_func_vco
	dd	module_func_lfo
	dd	module_func_cv
	dd	module_func_amp
	dd	module_func_mixer
	dd	module_func_vcf
	dd	module_func_lpf24
	dd	module_func_delay
	dd	module_func_att
	dd	module_func_resample
	dd	module_func_supersaw
	dd	module_func_dist
	dd	module_func_accent
	dd	module_func_output

; include the song itself from an external file
%include "song.inc"

;
; BSS
;
section .bss

tempfloat	resd	1
sample		resd	1

moddata		resd	NUM_CHANNELS*MAX_MODULES*32 ; output,mod,moddata*16,reserved*14

pitch		resd	NUM_CHANNELS
flags		resb	NUM_CHANNELS
patchptr	resd	NUM_CHANNELS

delaycount	resb	1
delaybuffer	resd	NUM_DELAYS*DELAYBUFFERSIZE

global songbuffer
songbuffer	resw	SONG_BUFFERLEN

;output		resd	NUM_CHANNELS*MAX_MODULES
;localdata	resd	NUM_CHANNELS*MAX_MODULES*16



;
; TEXT
;
section .text


; code for the synth modules
%include "modules.asm"


; this function renders the actual audio into the songbuffer in BSS
; with SONG_BUFFERLEN samples 
global render_song
render_song:

	; start the loop to fill the outputbuffer with rendered audio
	xor	ebx, ebx ; ebx = sample number
.sample_loop:
	push	ebx ; save sample number
	xor	edx, edx
	mov	eax, ebx
	idiv	dword [tickdivider]
	and	edx, edx
	jnz 	.synth
	mov	ecx, eax ; tick number in ecx
	shr	eax, 6 ; songpos is tick/64
	add	eax, eax
	lea	esi, [songdata+eax]

	; play notes
	xor	edx, edx
.channel_loop:
	mov	bl, byte [flags+edx] ; get flags to bl
        and     cl, 63
        jnz     .tick0_end
	xor	eax, eax
	lodsw
	and	al, al
        jz      .test_accent
        fld     dword [midi0]
.pitchmul:
        fmul    dword [midisemi]
        dec     al
        jnz     .pitchmul
        fdiv    dword [outputfreq]
        fstp    dword [pitch+edx*4]
        mov     bl, [seqmask+edx]
.test_accent:
	xchg 	al, ah
	test	al, 0x40
        jz      .test_noteoff
	or	bl, FLAG_ACCENT
.test_noteoff:
	test 	al, 0x80
        jz      .test_patch
	or	bl, FLAG_NOTEOFF
.test_patch:
        and     al, 0x3f
        jz      .tick0_end
        mov     ax, [patchstart+eax*2-2]
        lea     eax, [patchdata+eax*4]
        mov     [patchptr+edx*4], eax
	or	bl, FLAG_LOAD_PATCH
.tick0_end:
        cmp     cl, 60
        jnz     .channel_done
	test	bl, FLAG_NOTEOFF
        jz      .channel_done
	and	bl, FLAG_TRIG|FLAG_ACCENT
.channel_done:
	mov	[flags+edx], bl ; put flags back
	add	esi, (SONG_LEN-1)*2
        inc     edx
        cmp     edx, NUM_CHANNELS
	jnz	.channel_loop

	; process synthesizer voices
; notes:
;
; as long as modulator commands are not implemented, the modulator
; value for each module is always the one from patch data. therefore,
; FLAG_LOAD_PATCH is useless and modulator value could instead
; be loaded from [patchptr+edx*4]+ebp*4 every time.
;
; kbd cv -module gets its modulator from [pitch+edx*4]
;
.synth:
	xor	edx, edx ; edx=voice
	mov	[sample], edx ; set mixed sample to zero
.voice_loop:
	; process the signal stack by looping through all the
	; modules
.mod_process:
	movzx	ebp, byte SONGDATA(seqvoice+edx)
	movzx	ebp, byte SONGDATA(synthstart+ebp*2)
	; ebp = ptr to synth start, index to synth data

	xor	ecx, ecx ;ecx = module index
	mov	edi, edx ; edx=channel
	shl 	edi, 13 ; 8192 bytes per channel, 128 bytes per module
	lea 	edi, SONGBSS(moddata+edi) ; edi = start of moddata for this channel
	mov	esi, edi
.mod_process_loop:
	; load a new patch?
	test	byte SONGBSS(flags+edx), FLAG_LOAD_PATCH
	jz	.mod_process_prepare
	; load value from patch to modulator
	mov	eax, SONGBSS(patchptr+edx*4)
	mov	eax, [eax+ecx*4]
	mov	[edi+4], eax

	; store modulator first to fpu
.mod_process_prepare:
	fninit
	fld	dword [edi+4] ; modulator at +4

	; collect input signal voltages to fpu
	pushad
	add 	ebp, ecx 
	mov 	eax, SONGDATA(modinputs+ebp*4)
	mov	ecx, 4
.mod_signal_loop:
	xor	ebx, ebx
	mov	bl, al ; ebx = input module index
	shl	ebx, 7
	fld 	dword [esi+ebx] ; output at offset 0
	shr	eax, 8 ; next byte to al
	loop	.mod_signal_loop
	popad

	movzx	ebx, byte SONGDATA(modtypes+ebp+ecx) ; ebx=module type [0..127]
	pushad
	mov 	eax, [edi+4]		; eax = modulator as integer
	mov	cl, [flags+edx]         ; cl = flags
	lea 	esi, [edi+8]            ; esi = data area starts at +8
	call 	SONGDATA(modfunctable+ebx*4)
	popad
	fst 	dword [edi] ; store output

	; update noise generator on each module loop
	mov	eax, SONGBSS(noise_x1)
	xor	eax, SONGBSS(noise_x2)
	add	SONGBSS(noise_x2), eax
	mov	SONGBSS(noise_x1), eax

	; next module
	inc	ecx
	sub 	edi,byte -128
	cmp	bl, 0fh
	jnz	.mod_process_loop
.mod_process_end:
	and	byte [flags+edx], 0x0f ; clear hard restart flags

	fadd	dword [sample]          ; mix previous channels in
	fstp	dword [sample]          ; and pop back to temp storage
	inc	edx			; next voice
	cmp	edx, NUM_CHANNELS
	jl	.voice_loop
	; all voices done, sample now has the final channel mix

	pop	ebx ; restore sample number
	fld	dword SONGBSS(sample)
	fmul	dword SONGDATA(samplemul)
	fistp	word SONGBSS(songbuffer+ebx*2)
	inc	ebx
	cmp	ebx, SONG_BUFFERLEN
	jl	.sample_loop

	;done, the buffer is now full of music. :)
	ret

;;
;; eof
;;