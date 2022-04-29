        .org    $0200

TEMP    = $BF
POINTL  = $CA                   ; LSB OF OPEN CELL
POINTH  = $CB                   ; MSB OF OPEN CELL

SRCL  = $CC                     ; LSB OF WORD LIST
SRCH  = $CD                     ; MSB OF WORD LIST

WORD_IDX_L = $CE                   ; LSB OF WORD LIST
WORD_IDX_H = $CF                   ; MSB OF WORD LIST

TMP_IDX_L = $D0                   ; LSB OF WORD LIST
TMP_IDX_H = $D1                   ; MSB OF WORD LIST

SAD     = $1740                 ; character to output
SBD     = $1742                 ; segment to output data
PADD    = $1741                 ; 6530 RIOT data direction


; ROM ROUTINES
OUTCH  = $1EA0

;0000
;5  1
;5  1
;6666
;4  2
;4  2
;3333

MAIN:
        JSR     RESET

        LDA     #<MESSAGE
        STA     POINTL
        LDA     #>MESSAGE
        STA     POINTH

        ; LOAD THIS WORD
        LDA     #$06
        STA     WORD_IDX_L

        LDA     #$00
        STA     WORD_IDX_H
 
        ;JSR     LOAD_STRING

        ;BRK
        ;NOP
        ;NOP

Z2:     JSR     STRD
        JMP     Z2


        ; A = WORD SET 
        ; X = WORD NUMBER
LOAD_STRING:
        PHA                     ; Save the register

        ; Reset the source pointer to the start of the word list

        LDA     #$10
        STA     SRCL
        LDA     #$20
        STA     SRCH 


        ;; 
        ;; MULTIPLY THE OFFSET BY SIX TO MATCH THE ACTUAL MEMORY location
        ;;

        ; FIRST, COPY THE VALUES TO A TEMP AREA
        LDA     WORD_IDX_L
        STA     TMP_IDX_L
        STA     $00
        LDA     WORD_IDX_H
        STA     TMP_IDX_H
        STA     $01

        ; NOW,lSHIFT IT ALL OVER BY TWO BITS 
        ; (OR, MULTIPLY BY 4)
        ASL     TMP_IDX_L
        ROL     TMP_IDX_H
        ASL     TMP_IDX_L
        ROL     TMP_IDX_H

        ; ADD THE ORIGINAL VALUE TO OUR NEW VALUE
        ; WHICH GIVES US A MULTIPLE OF 5

        CLC				; clear carry
	lda     WORD_IDX_L
	adc     TMP_IDX_L
	sta     TMP_IDX_L		; store sum of LSBs
	lda     WORD_IDX_H
	adc     TMP_IDX_H		; add the MSBs using carry from
	sta     TMP_IDX_H			; the previous calculation
  
        ; AND REPEAT MAKES 6
        CLC				; clear carry
	lda     TMP_IDX_L
	adc     SRCL
	sta     SRCL			; store sum of LSBs
	lda     TMP_IDX_H
	adc     TMP_IDX_L		; add the MSBs using carry from
	sta     SRCL			; the previous calculation
  

        LDY     #$00

LOAD_STRING2:
        LDA     (SRCL),Y
        STA     (POINTL),Y 

        INY 
        CPY     #$06
        BNE     LOAD_STRING2

        RTS

 
;
; THE STRING TO SHOW IS REFERENCED IN
; POINTL/POINTH

STRD:
        LDA     #$7F            ; CHANGE SEG
        STA     PADD            ; TO OUTPUT
        LDX     #$09            ; INIT DIGIT NUMBER
        LDY     #$00            ; FIRST STR CHAR
STRD2:
        LDA     (POINTL),Y      ; GET NEXT CHARACTER OF MESSAGE

        STA     SAD             ; OUTPUT SEGMENTS
        STX     SBD             ; OUTPUT DIGIT ENABLE
        JSR     PAUSE

        CPX     #$13
        BEQ     STRD1
        INX                     ; OUTPUT PORT ADVANCES BY TWO
        INX                     ; SINCE ITS A SINGLE BINARY DIGIT SHIFT
        INY                     ; MOVE TO THE NEXT CHARACTER

        JSR     PAUSE
        JMP     STRD2
STRD1:
        RTS


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


GIGAPAUSE:
        PHA
        TYA
        PHA

        LDY     #$FF
GIGAPAUSE1:
        JSR     PAUSE
        DEY
        BNE     GIGAPAUSE1
        PLA
        TAY
        PLA
        RTS


CLRMEM: LDA #$00                ;Set up zero value
CLRM1:  DEY                     ;Decrement counter
        STA (POINTL),Y          ;Clear memory location
        BNE CLRM1               ;Not zero, continue checking
        RTS                     ;RETURN


;
; Reset the game.
;

RESET:
        JMP     CLRMEM

MESSAGE:
        .BYTE $88, $C0, $88, $C0, $88, $C0
