;;
;; Komposter player (c) 2010 Firehawk/TDA
;;
;; This code is licensed under the MIT license:
;; http://www.opensource.org/licenses/mit-license.php
;;
;; Here is the actual main program which renders the song, creates a
;; playback device and starts audio playback.
;;
;; This example code uses MacOS X ABI and OpenAL but adapting to other x86
;; systems should be trivial.
;;
;;

; write rendered audio to file?
;%define WRITE_TO_FILE 1

; the frequency used to play the sample data 
%define OUTPUTFREQ 44100

; length of the data buffer - must be the same as in in player.asm
%define SONG_BUFFERLEN 75*OUTPUTFREQ

; openal constants
%define AL_FORMAT_MONO16 0x1101
%define AL_BUFFER 0x1009
%define AL_GAIN 0x100A


extern _printf
extern _alcOpenDevice
extern _alcCreateContext
extern _alcMakeContextCurrent
extern _alGenSources
extern _alGenBuffers
extern _alBufferData
extern _alSourcei
extern _alSourcef
extern _alSourcePlay


; these are from player.asm
extern render_song
extern songbuffer


;
; DATA
;
section .data
initing db "rendering audio...",10,0
playing db "playing..",10,0



%ifdef WRITE_TO_FILE
extern _fopen
extern _fwrite
extern _fclose
writing db 10,"writing to file...",10,0
fname db "playerout.raw",0
fmode db "w",0
%endif


;
; BSS
;
section .bss
dev     resd    1 ;*ALCdevice
con     resd    1 ;*ALCcontext
buffer  resd    1
source  resd    1

ppos	resd	1
fpos	resd	1

fhandle	resd	1


;
; TEXT
;
section .text
global _start
_main:
_start:

	; pad stack to a 16 byte boundary. this is required
	; by os x abi and not doing it will make things break
	; spectacularly
.stackpad:
        mov     eax, esp
        and     eax, 0x0000000f
        jz      .stackok
        push    byte 0
        jmp     .stackpad
.stackok:

	; print the rendering message
        sub     esp, 12
        push    initing
        call    _printf
        add     esp, 16 

        ; fill buffer
	call	render_song

	; write message
%ifdef WRITE_TO_FILE
	sub	esp, 12
	push 	writing
	call	_printf
	add	esp, 16

	; write to file
	sub	esp, 8
	push	fmode
	push	fname
	call	_fopen
	add	esp, 16
	mov	[fhandle], eax
	push	eax
	push	dword SONG_BUFFERLEN
	push	dword 2
	push	songbuffer
	call	_fwrite
	add	esp, 16
	sub	esp, 12
	push	dword [fhandle]
	call	_fclose
	add	esp, 16
	ret
%endif

        ; dev=alcOpenDevice(NULL);
        sub     esp, 12
        push    dword 0
        call    _alcOpenDevice
        add     esp, 16
        mov     [dev], eax
        
        ; con=alcCreateContext(dev, NULL);
        sub     esp, 8
        push    dword 0
        push    dword [dev]
        call    _alcCreateContext
        add     esp, 16
        mov     [con], eax

        ; alcMakeContextCurrent(con);
        sub     esp, 12
        push    dword [con]
        call    _alcMakeContextCurrent
        add     esp, 16

        ; alGenSources(1, &source);
        sub     esp, 8
        push    source
        push    dword 1
        call    _alGenSources
        add     esp, 16

        ; alGenBuffers(1, &buffer);
        sub     esp, 8
        push    buffer
        push    dword 1
        call    _alGenBuffers
        add     esp, 16

        ; alBufferData(buffer, AL_FORMAT_MONO16, buf, BUFFER_SIZE*2, 44100);
        sub     esp, 12
        push    dword OUTPUTFREQ
        push    dword SONG_BUFFERLEN*2
        push    songbuffer
        push    AL_FORMAT_MONO16
        push    dword [buffer]
        call    _alBufferData
        add     esp, 32

        ; alSourcei(source, AL_BUFFER, buffer);
        sub     esp, 4
        push    dword [buffer]
        push    dword AL_BUFFER
        push    dword [source]
        call    _alSourcei
        add     esp, 16

        ; alSourcef(sourceID, AL_GAIN, volume);
        sub     esp, 4
        push    dword __float32__(1.0)
        push    dword AL_GAIN
        push    dword [source]
        call    _alSourcef
        add     esp, 16

        ; alSourcePlay(source);
        sub     esp, 12
        push    dword [source]
        call    _alSourcePlay
        add     esp, 16

	; print the playing message
        sub     esp, 12
        push    playing
        call    _printf
        add     esp, 16 

	; loop forever
.loopin:
        jmp     .loopin

;;
;; eof
;;
