INCLUDE "gbhw.asm"

MBC5_bank_low EQU $2000
MBC5_bank_high EQU $3000

audio_buffer EQU $c100
line_buffer EQU ((audio_buffer >> 8) * $101)

current_line EQU $94 ; For backup, current_line is stored in SP
current_bank EQU $96 ; For backup, current_bank is stored in b (Except for the 9th bit)


frame_repeat EQU $a0
; Address to jump to on repeat
repeat_bank EQU $a1
repeat_line EQU $a3

audio_bank EQU $a5 ; Since audio isn't that big, we use only one byte for bank number
audio_address EQU $a6
first_sample_backup EQU $a8

compression_jr EQU $fd


SECTION "Player", ROM0[0]
Bank0:
; Write changed values (Up to 9) compared to the previous line
rept 9
    pop de
    ld l, d
    ld [hl], e
endr
; Decrease all values by 1
    ld l, h
    dec [hl]
rept 19
    inc l
    dec [hl]
endr

WaitForInterrupt::
    ; Wait for interrupt
    halt
    xor a
    ldh [rIF], a

    ; Update PCM sample
    ld h, audio_buffer >> 8
    ldh a, [rLY]
    ld l, a
    cp 143
    ld a, [hl]
    ldh [rNR50], a

    ; Render the line by modifying SCY
    ld l, h ; hl now points to line_buffer
rept 20
    ld a, [hli]
    ld [c], a ; c = rSCY
endr
    jr z, _VBlank
    ld l, h ; Due to how line_buffer is defined, hl now points to line_buffer
    
; The main loop. HL will point to either a frame start, or a row start
Main::
    ; When entering Main, the following is assumed:
    ; - c is rSCY
    ; - b is the lowest 8 bits of bank
    ; - sp points to current video position in ROM1
    ; - hl = line_buffer
    
    ; Read the first byte at sp
    pop de
    dec sp
    ; Valid values for e:
    ; $00 - Uncompressed, no new bank
    ; $01 - Uncompressed, data starts at $4000 on new bank
    ; $02 - Compressed, 9 changes
    ; $08 - Compressed, 8 changes
    ; $0e - Compressed, 7 changes
    ; $14 - Compressed, 6 changes
    ; $1a - Compressed, 5 changes
    ; $20 - Compressed, 4 changes
    ; $26 - Compressed, 3 changes
    ; $2c - Compressed, 2 changes
    ; $32 - Compressed, 1 changes
    ; $38 - Compressed, no changes
    sra e
    jr nz, Compressed ; If neither 0 or 1, means line is compressed
    jr nc, NoNewBank; ; If zero, means no bank switch needed
    ; If 1, means bank switch required
    
    ; b is the current bank
    inc b
    ld h, MBC5_bank_low >> 8
    ld [hl], b ; Write to $20xx to bank switch
    ld sp, $4000 ; Reset SP
    ld h, l ; Due to how line_buffer is defined, hl now points to line_buffer

    
NoNewBank::
    ; Copy the current line (20 bytes) to line_buffer
rept 10
    pop de
    ld a, e
    ld [hli], a
    ld a, d
    ld [hli], a
endr
    jr WaitForInterrupt

_VBlank::
    jp VBlank
Compressed::
    ld a, e
    ldh [compression_jr + 1], a
    jp $ff00 + compression_jr
; Header
ds $100 - (@ - Bank0)

_Start::
    di
    jp Start

; Start
ds $150 - (@ - Bank0)
    
Start::
    ; Increase the CPU speed from 4MHz to 8MHz
    ld a, 1
    ldh [rKEY1], a
    stop

    ; Init the stack for the initialization routines
    ld sp, $fffe

    ; Other inits
    call InitAPU
    call LCDOff
    call LoadGraphics
    call CreateMap
    call CreateAttributeMap

    ; Start Playing
    jp StartPlayback

WaitVBlank::
    ldh a, [rLY]
    cp 144
    jr nz, WaitVBlank
    ret

InitAPU::
    ; Reset the APU
    xor a
    ldh [rNR52], a
    ld a, $80
    ldh [rNR52], a
    ; Turn all DACs on
    ldh [rNR12], a
    ldh [rNR22], a
    ldh [rNR30], a
    ldh [rNR42], a
    ; Put all channels on both left and right
    ld a, $FF
    ldh [rNR51], a
    ret

LCDOff::
    call WaitVBlank
    ldh a, [rLCDC]
    and $7F
    ldh [rLCDC], a
    ret

LoadGraphics::
    ld de, $8000
    ld b, PixelStructureEnd - PixelStructure
    ld hl, PixelStructure
.loop
    ld a, [hli]
    ld [de], a
    inc de
    dec b
    jr nz, .loop
    ret

CreateMap::
    ld hl, $9800
    ld a, 0
    ld c, 32
    xor a
.loopY
    ld b, 32
.loopX
    ld [hli], a
    dec b
    jr nz, .loopX
    inc a
    and 3
    dec c
    jr nz, .loopY
    ret

CreateAttributeMap::
    ld a, 1
    ldh [rVBK], a
    ld hl, $9800
    ld a, 7
    ld c, 32
.loopY
    ld b, 32
.loopX
    ld [hli], a
    dec b
    jr nz, .loopX
    dec c
    ld a, c
    dec a
    rra
    rra
    and 7
    jr nz, .loopY
    ret

PixelStructure::
    db %00000000
    db %00000000
    db %00011111
    db %00000000
    db %00000000
    db %00011111
    db %00011111
    db %00011111
    
    db %11100000
    db %00000000
    db %11111111
    db %00000000
    db %11100000
    db %00011111
    db %11111111
    db %00011111
    
    db %00000000
    db %11100000
    db %00011111
    db %11100000
    db %00000000
    db %11111111
    db %00011111
    db %11111111
    
    db %11100000
    db %11100000
    db %11111111
    db %11100000
    db %11100000
    db %11111111
    db %11111111
    db %11111111
    
    db %00110011
    db %00001111
    db %00000111
    db %00000000
    db %00000000
    db %00000111
    db %00000111
    db %00000111
    
    db %11111000
    db %00000000
    db %11001100
    db %00001111
    db %11111000
    db %00000111
    db %11111111
    db %00000111
    
    db %00000000
    db %11111000
    db %00000111
    db %11111000
    db %00110011
    db %11110000
    db %00000111
    db %11111111
    
    db %11111000
    db %11111000
    db %11111111
    db %11111000
    db %11111000
    db %11111111
    db %11001100
    db %11110000

PixelStructureEnd::

StartPlayback:

    ; Enable timer interrupt
    ld a, 4
    ldh [rIE], a
    
    ; Set SCX to 4. When HandleFrameRepeatAndUnshiftFramebuffer runs,
    ; it will set it back to 0 and consider this (HW) frame as the start
    ; of a (video) frame.
    ldh [rSCX], a 
    
    ; Set up the compression JR instruction
    ld a, $18
    ldh [compression_jr], a
    
    ; Init variables
    xor a
    ldh [current_bank + 1], a
    
    ld hl, audio_buffer
    ld c, 154
.loop
    ld [hli], a
    dec c
    jr nz, .loop
    
    ld [$3000], a
    
    ; Audio starts at 1:4001
    inc a
    ldh [audio_bank], a
    ldh [audio_address], a
    ldh [frame_repeat], a ; Set frame repeat to 1 so the VBlank functions load
                          ; a new frame
                          
    ld [$2000], a
    
    ld a, $40
    ldh [audio_address + 1], a
    
    ; Fill line_buffer with $101 - 144. When HandleFrameRepeatAndUnshiftFramebuffer
    ; runs, it will add 144 to all bytes, setting them to the expected initial
    ; value of $01.  (Note: the encoder currently does not make use of this property)
    ld a, $101 - 144
    ld c, 20
    ld l, h ; hl = line_buffer
.loop2
    ld [hli], a
    dec c
    jr nz, .loop2
    
    ld c, rSCY & $FF ; C is assumed to be SCY when entring Main
    ; Audio comes first, first byte at ROM1 is the first video bank
    ld a, [$4000]
    ld b, a ; B is assumed to be the current bank
    ld [$2000], a
    ld sp, $4000 ; Point SP to video start
    ld hl, line_buffer
    
    ; Enable LCD
    ldh a, [rLCDC]
    or $80
    ldh [rLCDC], a
    
.lyloop
    ldh a, [rLY]
    cp 143
    jr nz, .lyloop
    
    ; Exactly 2 NOPs are needed to sync properly on both CGB-D and newer and CGB-C and older
    nop
    nop
    ; Configure timer
    ld [rDIV], a ; Synchronize DIV
    ld a, $100-(912 / 16) ; Configure modulo so we tick every 912 clocks (The length of a scanline in double speed mode)
    ldh [rTMA], a
    ld a, $c8 ; Configure the initial value of TIMA so we overflow at the right timing
    ldh [rTIMA], a
    ld a, 5 ; Enable, tick every 16 CPU clocks
    ldh [rTAC], a
    ; Clear interrupts
    xor a
    ldh [rIF], a
    jp VBlank

; Align to $100
ds $100 - ((@ - Bank0) & $FF)
VBlankJumpTable::
    dw HandleFrameRepeatAndUnshiftFramebuffer ; 144
    dw HandleSCXAndLoadPalette ;145
    dw LoadPalette ; 146
    dw CopyPCM24 ; 147
    dw CopyPCM26 ; 148
    dw CopyPCM26 ; 149
    dw CopyPCM26 ; 150
    dw CopyPCM26 ; 151
    dw CopyPCMLY152 ; 152
    ; dw ReturnToMain ; During line 153 LY almost always reads 0, so it's handled outside of this table
    
VBlank::
    ; Wait for interrupt
    halt
    xor a
    ldh [rIF], a
    
    ; Update PCM sample
    ld h, audio_buffer >> 8
    ldh a, [rLY]
    ld l, a
    cp 143 ; Result not used, only here to match the cycle count in MainLoop
    ld a, [hl]
    ldh [rNR50], a
    
    ld a, l
    sub 144
    jr c, ReturnToMain
    add a
    ld l, a
    ld h, VBlankJumpTable >> 8
    ld a, [hli]
    ld h, [hl]
    ld l, a
    jp hl
    


ReturnToMain::

    ld a, b
    ld [$2000], a
    ldh a, [current_bank + 1]
    ld [$3000], a
    ld de, 153
    ld hl, sp+0
    add hl, de
    ld a, $80
    cp h
    jr nz, NoAudioBankSwitch
    
    ldh a, [audio_bank]
    inc a
    ldh [audio_bank], a
    ld sp, $4000
NoAudioBankSwitch::
    ld [$ff00 + audio_address], sp
    ld hl, $ff00 + current_line
    ld a, [hli]
    ld h, [hl]
    ld l, a
    ld sp, hl
    ldh a, [first_sample_backup]
    ld [audio_buffer], a
    ld hl, line_buffer
    ld c, rSCY & $ff
    jp Main

; We probably have enough CPU cycles to implement compression

CopyPCMLY152::
    ld a, [audio_buffer]
    ldh [first_sample_backup], a    
CopyPCM26::
    ld h, audio_buffer >> 8
    ld l, c
rept 26 / 2
    pop de
    ld [hl], e
    inc l
    ld [hl], d
    inc l
endr
    ld c, l
    jp VBlank ; Not implemented

CopyPCM24::
    ld [$ff00 + current_line], sp
    ld a, b
    ldh [current_bank], a
    ld hl, $ff00 + audio_bank
    ld a, [hli]
    ld [$2000], a
    xor a
    ld [$3000], a
    ld a, [hli]
    ld h, [hl]
    ld l, a
    ld sp, hl
    ld hl, audio_buffer

rept 24 / 2
    pop de
    ld [hl], e
    inc l
    ld [hl], d
    inc l
endr
    ld c, l
    jp VBlank ; Not implemented

HandleFrameRepeatAndUnshiftFramebuffer::
    ld hl, line_buffer
    ld d, 144
rept 20
    ld a, [hl]
    add a, d
    ld [hli], a
endr
    ldh a, [rSCX]
    and a
    jp z, VBlank ; A video frame is 2 GB frames
    ld a, b
    inc a
    jr nz, BankNotFF
    ; A frame must not start at bank $ff
    ld [MBC5_bank_low], a
    ld b, a
    inc a
    ld [MBC5_bank_high], a
    ldh [current_bank + 1], a
    ld sp, $4000
BankNotFF::
    ldh a, [frame_repeat]
    dec a
    jr nz, RepeatFrame
    pop de
    dec sp
    ld a, e
    and a
    jp z, RestartPlayback
    ldh [frame_repeat], a
    ld [$ff00 + repeat_line], sp
    ld a, b
    ldh [repeat_bank], a
    ldh a, [current_bank + 1]
    ldh [repeat_bank + 1], a
    jp VBlank
RepeatFrame::
    ldh [frame_repeat], a
    ld hl, $ff00 + repeat_bank
    ld a, [hli]
    ld [MBC5_bank_low], a
    ld b, a
    ld a, [hli]
    ld [MBC5_bank_high], a
    ld a, [hli]
    ld h, [hl]
    ld l, a
    ld sp, hl
    jp VBlank

HandleSCXAndLoadPalette:
    ldh a, [rSCX]
    xor 4
    ldh [rSCX], a
LoadPalette:
    ldh a, [rSCX]
    and a
    jp nz, VBlank ; A video frame is 2 HW frames
    ; When here, SP points to a color. Since color is stored in Big Endian
    ; mode, that byte is not allowed to be $ff. If we do point to $ff,
    ; it means we need to bank switch.
    pop de
    dec sp
    dec sp
    ld a, e
    inc a
    jp nz, NoBankSwitch
    
    inc b
    ld h, MBC5_bank_low >> 8
    ld [hl], b ; Write to $20xx to bank switch
    ld sp, $4000 ; Reset SP
NoBankSwitch:
    ld hl, rBGPD
rept 16
    pop de
    ld [hl], d
    ld [hl], e
endr
    jp VBlank


RestartPlayback::
    ; Clear palette
    ld hl, rBGPD
    xor a
rept 64
    ld [hl], a
endr
    ; Set SCX to 4. When HandleFrameRepeatAndUnshiftFramebuffer runs,
    ; it will set it back to 0 and consider this (HW) frame as the start
    ; of a (video) frame.
    ld a, 4
    ldh [rSCX], a 


    ; Init variables
    xor a
    ldh [current_bank + 1], a

    ld hl, audio_buffer
    ld c, 154
.loop
    ld [hli], a
    dec c
    jr nz, .loop

    ld [$3000], a

    ; Audio starts at 1:4001
    inc a
    ldh [audio_bank], a
    ldh [audio_address], a
    ldh [frame_repeat], a ; Set frame repeat to 1 so the VBlank functions load
                          ; a new frame

    ld [$2000], a

    ld a, $40
    ldh [audio_address + 1], a

    ; Fill line_buffer with $101 - 144. When HandleFrameRepeatAndUnshiftFramebuffer
    ; runs, it will add 144 to all bytes, setting them to the expected initial
    ; value of $01. (Note: the encoder currently does not make use of this property)
    ld a, $101 - 144
    ld c, 20
    ld l, h ; hl = line_buffer
.loop2
    ld [hli], a
    dec c
    jr nz, .loop2

    ld c, rSCY & $FF ; C is assumed to be SCY when entring Main
    ; Audio comes first, first byte at ROM1 is the first video bank
    ld a, [$4000]
    ld b, a ; B is assumed to be the current bank
    ld [$2000], a
    ld sp, $4000 ; Point SP to video start
    ld hl, line_buffer


.lyloop
    ldh a, [rLY]
    cp 144
    jr nz, .lyloop

    xor a
    ldh [rIF], a
    jp VBlank