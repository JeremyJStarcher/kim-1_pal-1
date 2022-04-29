        .ORG    $0200

COMPRESSED_WORD_SIZE = 6
CLEAR_WORD_SIZE = 6

DATA_ADDR = $2000
WORDS_ADDR = DATA_ADDR+10
WORD_TAIL_IDX_ADDR = DATA_ADDR

; Index of the current tail.  Counts down to 0.
CURRENT_TAIL_IDX = $00
;CURRENT_TAIL_IDX = $01


; Pointer to the start of the word list.
WORDLIST_PTR  = $04
; WORDLIST_PTR+1 = $05

; Index of the current word (chosen by random)
WORD_IDX = $06
; WORD_IDX_L+1 = $07

CURRENT_TAIL_PTR = $08
;CURRENT_TAIL_PTR = $09


; Seed values for the RND
SEED0 = $B0
SEED1 = $B1
SEED2 = $B2
SEED3 = $B3

; Misc temp values, but also used by the RNG
TMP = $B4
TMP1 = $B5
TMP2 = $B6
TMP3 = $B7

; The 'MOD' or upper limit of the RNG.
; Is destroyed by the fall.
MOD = $B8
; MODH = $B9

; The RANDOM16 output
RND = $BA
; RND_H = $BB


; The 'USER' timers
;T0001 = $1704
;T0008 = $1705
;T0064 = $1706
;T1024 = $1707
;TSTATUS = $1707
;TREAD_TIME = $1706

; The 'SYSTEM' timers
T0001 = $1744
T0008 = $1745
T0064 = $1746
T1024 = $1747
TSTATUS = $1747
TREAD_TIME = $1746

SAD     = $1740                 ; character to output
SBD     = $1742                 ; segment to output data
PADD    = $1741                 ; 6530 RIOT data direction


; ROM ROUTINES
OUTCH   = $1EA0
PRTBYT  = $1E3B                 ; byte in A register
CRLF    = $1E2F
OUTSP   = $1E9E
GETKEY  = $1F6A

        JMP     MAIN

SPEED: 
        .BYTE 127

CLEARTEXT:
        .BYTE $00, $00, $00, $00, $00, $00

HIDDENTEXT:
        .BYTE $00, $00, $00, $00, $00, $00


; What order should we reveal the screet word?
REVEAL_ORDER:
        .BYTE $00, $01, $02, $03, $04, $05


SHUFFLE_TMP:
        .BYTE $00


MAIN:
        JSR     RESET

        ; Y = LSB
        ; X = MSB
        LDY     WORD_TAIL_IDX_ADDR
        LDX     WORD_TAIL_IDX_ADDR+1

INNERLOOP:
        ;do work here

        STY     CURRENT_TAIL_IDX
        STX     CURRENT_TAIL_IDX+1

        ; MOD gets destroyed by the RANDOM16
        STY     MOD
        STX     MOD+1

        ; And save the same values to get the pointer
        STY     WORD_IDX
        STX     WORD_IDX+1
        JSR     CALCULATE_WORD_PTR
        LDA     WORDLIST_PTR
        STA     CURRENT_TAIL_PTR
        LDA     WORDLIST_PTR+1
        STA     CURRENT_TAIL_PTR+1

        JSR     RANDOM16

        LDY     RND
        STY     WORD_IDX
        LDX     RND+1
        STX     WORD_IDX+1

        JSR     CALCULATE_WORD_PTR
        JSR     LOAD_WORD

        ; JSR     OUT_STRING
        ; JSR     OUTSP
        ; JSR     OUTSP
        ; JSR     OUTSP

        ; LDA     RND+1
        ; JSR     PRTBYT
        ; LDA     RND
        ; JSR     PRTBYT
        ; JSR     CRLF

        JSR     SWAP_WORDS

        JSR     SHUFFLE_REVEAL

        JSR     PLAY_ROUND

        LDY     CURRENT_TAIL_IDX
        LDX     CURRENT_TAIL_IDX+1

        ; If the inner loop has not rolled past zero, repeat
        DEY
        CPY     #$FF
        BNE     INNERLOOP

        ; If the outer loop has not rolled past zero, repeat
        ; No need to reset 'Y', it will keep the value '$FF'
        DEX
        CPX     #$FF
        BNE     INNERLOOP

        RTS

SHUFFLE_REVEAL:
        LDY     #CLEAR_WORD_SIZE-1 
SHUFFLE_REVEAL1:
        STY     SHUFFLE_TMP

        ; Y now holds how many characters left
        ; We'll just re-use the 16 bit random number generator
        ; and ignore the high byte entirely
        STY     MOD
        LDA     #$00
        STA     MOD+1
        
        JSR     RANDOM16

        LDY     RND             ; Get the random index
        LDA     REVEAL_ORDER,Y  ; And then the value from that index
        STA     TMP             ; And stash it away

        LDY     SHUFFLE_TMP     ; Get the order we are walking through
        LDA     REVEAL_ORDER,Y  ; Get -that- value

        LDY     RND             ; Get our random index again
        STA     REVEAL_ORDER,Y  ; Save it ordered slot

        LDY     SHUFFLE_TMP
        LDA     TMP
        STA     REVEAL_ORDER,Y
        
        DEY                           
        BPL     SHUFFLE_REVEAL1         ; 
        RTS


PLAY_ROUND:
        LDY     #CLEAR_WORD_SIZE-1      ; Go through all the letters
SHOW_NEXT_LETTER:
        STY     TMP                     ; Save Y

        LDA     REVEAL_ORDER,Y          ; Get the next position to reveal
        TAY                             ; move it to Y
        LDA     CLEARTEXT,Y             ; Get the clear text letter
        STA     HIDDENTEXT,Y            ; And show it

        CMP     #$00                    ; is it a character not to show?
        BEQ     SKIP_SHOW               ; Skip showing it

        LDX     SPEED
FLASHO:                         ; Flash outer loop
        STX     TMP1
        LDA     #$FF
        STA     T0064
FLASH:
        JSR     STRD
        LDA     TSTATUS
        BPL     FLASH
        LDX     TMP1
        DEX
        BPL     FLASHO

SKIP_SHOW:
        LDY     TMP
        DEY                             ; Dec the pointer
        BPL     SHOW_NEXT_LETTER        ; Branch if Y does not role past zero
        RTS

SWAP_WORDS:

        LDY     #$00
SWAP_WORDS3:
        STY     TMP3

        LDA     (WORDLIST_PTR),Y
        TAX
        LDA     (CURRENT_TAIL_PTR),Y
        STA     (WORDLIST_PTR),Y
        TXA
        STA     (CURRENT_TAIL_PTR),Y

        LDA     (TMP),Y                 ; Look up the actual pattern

        LDY     TMP3
        INY
        CPY     #COMPRESSED_WORD_SIZE
        BNE     SWAP_WORDS3
        RTS

CALCULATE_WORD_PTR:
        ; Set the pointer to the base address of the word list
        LDA     #<WORDS_ADDR
        STA     WORDLIST_PTR
        LDA     #>WORDS_ADDR
        STA     WORDLIST_PTR+1

        ; WORDLIST_PTR += (WORD_IDX * 6)
        ;
        ; Adjust the pointer through a loop of repeated ADC.

        LDY     #6                      ; Loop six times
CALCULATE_WORD_PTR2:
        CLC                             ; clear carry
        LDA     WORD_IDX                ; Get the lead LSB
        ADC     WORDLIST_PTR            ; Add to our base value (store in accumulator) (may set carry)
        STA     WORDLIST_PTR            ; Save back to the base value LSB
        LDA     WORD_IDX+1              ; Get the high value
        ADC     WORDLIST_PTR+1          ; Add to the base high value (PLUS carry bit from the LSB)
        STA     WORDLIST_PTR+1          ; Save back to the base MSB
        DEY                             ; DEC Y
        BNE     CALCULATE_WORD_PTR2     ; Loop if needed

        RTS

LOAD_WORD:
 
        ; Now that we found the string in RAM, copy it to our
        ; display buffer.
        LDY     #COMPRESSED_WORD_SIZE-1 ; Repeat six times stopping AFTER 0
LOAD_WORD3:
        STY     TMP3                    ; Transfer to the X register
        LDA     (WORDLIST_PTR),Y        ; Load the character from the word list
        TAY
        LDA     CODE_TO_LED,Y           ; Look up the actual pattern
        LDY     TMP3
        
        STA     CLEARTEXT,Y             ; Save in the local buffer
        
        CMP     #$00                    ; Compare to the 'blank' character
        BEQ     LOAD_WORD4              ; If it is, just save that
        LDA     CODE_HIDDEN             ; Get the 'unknown character' code

LOAD_WORD4:
        STA     HIDDENTEXT,Y            ; Save in the local buffer

        DEY                             ; Dec the pointer
        BPL     LOAD_WORD3              ; Branch if Y does not role past zero

        RTS                             ; Return from the subtitle


;
; THE STRING TO SHOW IS REFERENCED IN
; HTEXT_PTR/HTEXT_PTR_H

STRD:
        ; Set the data direction register to OUTPUT for the pins that drive the LEDs
        LDA     #$7F                    ; PINS to set as output 01111111
        STA     PADD                    ; Save in the data direction register
        LDX     #$09                    ; Point to the first LED (1001)
        LDY     #$00                    ; Set the index into the buffer
STRD2:
        LDA     HIDDENTEXT,Y            ; Get next character of the message

        STA     SAD                     ; Turn on the segments for the LED
        STX     SBD                     ; And turn on that particular LED
        JSR     PAUSE                   ; And wait a bit (else the LEDs "bleed")

        CPX     #$13                    ; Did we do the last LED?
        BEQ     STRD1                   ; Yes, so branch
        INX                             ; Add two the the output port
        INX                             ; (.. since we can't shift the bits)
        INY                             ; Advance the pointer to the next character

        JSR     PAUSE
        JMP     STRD2                   ; Repeat the loop
STRD1:
        ; Scan the keyboard. Not because we need the key
        ; but because it cleanly turns the display off
        JSR     GETKEY

        RTS                             ; Return from subroutine

; Busyloop pause timed for the display.  This pause should match the pause
; of the built-in KIM-1 ROM routines.
;
; Keeps the timer clear to do longer-running timing events

PAUSE:
        PHA
        TYA
        PHA

        LDY   #$7F
PAUSE1: DEY
        BNE   PAUSE1
        PLA
        TAY
        PLA
        RTS

;
; Reset the game.
;

RESET:
        ; Seed the random number generator. Ick, but we'll find a better way.
        LDA     T0001
        STA     SEED0
        LDA     T0064
        STA     SEED1
        LDA     T0001
        STA     SEED2
        LDA     T0064
        STA     SEED3

        RTS

;0000
;5  1
;5  1
;6666
;4  2
;4  2
;3333

; Translate from the short code in the word list to the LED pattern
CODE_TO_LED:
CODE_BLANK:
        .BYTE $00 ; ' '
        .BYTE $f7 ; 'a'
        .BYTE $fc ; 'b'
        .BYTE $b9 ; 'c'
        .BYTE $de ; 'd'
        .BYTE $f9 ; 'e'
        .BYTE $f1 ; 'f'
        .BYTE $ef ; 'g'
        .BYTE $f4 ; 'h'
        .BYTE $84 ; 'i'
        .BYTE $9e ; 'j'
        .BYTE $86 ; 'l'
        .BYTE $dc ; 'o'
        .BYTE $f3 ; 'p'
        .BYTE $ed ; 's'
        .BYTE $be ; 'u'
        .BYTE $ee ; 'y'
        .BYTE $d4 ; 'n'
        .BYTE $d0 ; 'r'
        .BYTE $f8 ; 't'
CODE_HIDDEN:
        .BYTE $08 ; '_'

        .include "rand.s"
        ; .include "debug.s"