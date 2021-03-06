;------------------------------------------
; PURPOSE : Key breaker' draw tools
; SYSTEM  : Turbo Assembler Ideal Mode
; AUTHOR  : Almog Hamdani
;------------------------------------------

;-----  This segment will hold the contents of the file we open
SEGMENT FILEBUF para public
        DB 65200 dup (?)
ENDS

PROC DrawRectangle
;-----  Check if start point is out of bounds, if yes return
	cmp [StartX], ScreenWidth
	jb @@CheckNegX
	ret ; If out of bounds, return

@@CheckNegX:
;-----	Check if the x is negetive
	cmp [StartX], 0
	jl @@NegX
	jmp @@CheckOverflow

@@NegX:
;----- If negetive reset it and remove it's value from the length
	mov ax, [StartX]
	add [SizeWidth], ax
	mov [StartX], 0

@@CheckOverflow:
	mov ax, [StartX]
	add ax, [SizeWidth]
	cmp ax, ScreenWidth

	jbe @@SetY

	mov ax, ScreenWidth
	sub ax, [StartX]
	mov [SizeWidth], ax

@@SetY:
;-----  Set Y as the start of the rectangle
        mov ax, [StartY]
        mov [Y], ax

@@Rectangle:
;-----  Set X as the start of the rectangle
        mov ax, [StartX]
        mov [X], ax

        mov cx, [SizeWidth] ; Set for loop

@@DrawHLine:
        call PutPixel
        inc [X] ; Set next pixel
        loop @@DrawHLine

        inc [Y] ; Set next line

;-----  Check if didn't reach last line
        mov ax, [SizeHeight]
        add ax, [StartY]
        cmp [Y], ax
        jbe @@Rectangle

        ret
ENDP DrawRectangle

;------------------------
;PaintScreen - Paints the entire screen in 1 color
;------------------------
;Input:
;       Color <- The color to paint with
;Output:
;       The entire screen with the color
;Registers:
;       AX, BX, CX, DX, SI, DI, ES
;------------------------
PROC PaintScreen
        mov [X], 0
        mov [Y], 0

@@Paint:
        call PutPixel

        inc [X]
        cmp [X], 320 * 200
        jne @@Paint

        ret
ENDP PaintScreen

;------------------------
;ReadPCX - Reads a PCX file
;------------------------
;Input:
;       FileName <- The PCX file name
;Output:
;       FILEBUF <- The contents of the file, FileSize <- The size of the file
;Registers:
;       AX, BX, CX, DX
;------------------------
PROC ReadPCX
;-----  Try open file using interrupt
        mov ah, 3Dh ; Interrupt entry
        mov al, 0h ; Open for read only
        lea dx, [FileName]
        int 21h

;-----  Error occurred
        jc @@Error

        mov [FileHandle], ax

;-----  Set file pointer to it's end
        mov ah, 42h  ; Interrupt Entry
        mov al, 2h ; Setting offset from end of file
        mov bx, [FileHandle]

        ; Set offset to 0:0
        xor dx, dx
        xor cx, cx

        int 21h

;-----  Error occurred
        jc @@Error

;-----  Set file size
        mov [FileSize], ax

;-----  Return file pointer to start
        mov ah, 42h  ; Interrupt Entry
        mov al, 0h ; Setting offset from start of file
        mov bx, [FileHandle]

        ; Set offset to 0:0
        xor dx, dx
        xor cx, cx

        int 21h

;-----  Error occurred
        jc @@Error

;-----  Read all file into FILEBUF segment
        push ds ; Save original data segment

        mov cx, [FileSize] ; Tell interrupt to read all the file
        mov bx, [FileHandle]

        ; Set data segment as FILEBUF segment
        mov ax, FILEBUF
        mov ds, ax
        xor dx, dx ; Set offset 0

        mov ah, 3Fh ; Interrupt entry

        int 21h

;-----  Error occurred
        jc @@Error

        pop ds ; Return data segment

;-----  Close file
        mov ah, 3Eh ; Interrupt entry
        mov bx, [FileHandle]

        int 21h

;-----  Error occurred
        jc @@Error
        ret

@@Error:
;-----  Set text mode
        mov ax, 3h
        int 10h

;-----  Print error
        mov ah, 9h
        lea dx, [PCXErrorMSG]
        int 21h
        jmp Exit
ENDP

;------------------------
;DrawPCX - Draws a pcx to a specific location on the screen (Currently only supports default pallete)
;------------------------
;Input:
;       StartX <- The X to start draw in, StartY <- The Y to start draw in, FileName <- The PCX file name
;Output:
;       The PCX on the screen
;Registers:
;       AX, BX, CX, DX, SI, DI, ES
;------------------------
PROC DrawPCX
        call ReadPCX

;-----  Set extra segment as file buffer
        mov ax, FILEBUF
        mov es, ax
        mov si, 128 ; Set si to point at the start of the image data

;-----  Get image' width
        mov ax, [es:8h] ; Width is in the 8h pos
        inc ax ; Plus 1
        mov [ImageWidth], ax

;-----  Get image' height
        mov ax, [es:0Ah] ; Height is in the 6h pos
        inc ax ; Plus 1
        mov [ImageHeight], ax

;-----  Set starting position
        mov ax, [StartX]
        mov [X], ax
        mov ax, [StartY]
        mov [Y], ax

@@GetByte:
        mov al, [es:si]
        mov [Color], al
        inc si ; Point to next byte
        cmp [Color], 192 ; Check if there is a seq
        jb @@DrawNormal

        sub [Color], 192 ; In cx there is the amount of pixels to write
        xor ch, ch ; Reset cx
        mov cl, [Color] ; Set in cx the length to use for loop

        mov al, [es:si]
        mov [Color], al
        inc si ; Point to next byte

@@DrawSeq:
;-----  Set extra segment as video memory
        mov ax, 0A000h
        mov es, ax

        call PutPixel

;-----  Set extra segment as file buffer
        mov ax, FILEBUF
        mov es, ax

        inc [X] ; Set next x

;-----  Check if we got to the end of the line
        mov bx, [StartX]
        add bx, [ImageWidth]
        cmp bx, [X]
        je @@NewLine

        loop @@DrawSeq

        jmp @@GetByte

@@DrawNormal:
;-----  Set extra segment as video memory
        mov ax, 0A000h
        mov es, ax

        call PutPixel

;-----  Set extra segment as file buffer
        mov ax, FILEBUF
        mov es, ax

        inc [X] ; Set next x

;-----  Check if we got to the end of the line
        mov bx, [StartX]
        add bx, [ImageWidth]
        cmp bx, [X]
        je @@NewLine

        jmp @@GetByte

@@NewLine:
        mov ax, [StartX]
        mov [X], ax ; Set x to start

        inc [Y] ; Set next line

;-----  Check if the width of the image is odd or even, if it is odd, increase ptr by 1 because of 0 in the end, else continue
        mov ax, [ImageWidth]
        and ax, 1
        cmp ax, 1
        jne @@LineCheck

        inc si

@@LineCheck:
;-----  Check if the line was the last line
        mov bx, [StartY]
        add bx, [ImageHeight]
        cmp bx, [Y]
        je @@End

        jmp @@GetByte

@@Error:
;-----  Set text mode
        mov ax, 3h
        int 10h

;-----  Print error
        mov ah, 9h
        lea dx, [PCXErrorMSG]
        int 21h
        jmp Exit

@@End:
;-----  Return video memory
        mov ax, 0A000h
        mov es, ax

        ret
ENDP DrawPCX

;------------------------
;ClearSprite - Removes sprite from screen
;------------------------
;Input:
;       X, Y, Color, CX <- Sprite' height, DX <- Sprite' width
;Output:
;       None
;Registers:
;       AX, CX, DX, SI, DI, ES
;------------------------
PROC ClearSprite
        push dx ; Save sprite' original width

@@Cycle:
        call PutPixel
        inc [X] ; Move pixel' x by 1
        dec dx ; Decrease width by 1
        jnz @@Cycle ; While we didn't reach 0 (End of line), keep on cycle

        pop dx ; Return sprite' original width
        push dx ; Save width in stack

        sub [X], dx ; Set ax to start of line again
        inc [Y] ; Point to a new line

        dec cx ; Decrease sprite' height
        jnz @@Cycle

        pop dx ; Clean stack
        ret
ENDP ClearSprite

;------------------------
;PrintSprite - Prints a sprite to the screen
;------------------------
;Input:
;       StartX, StartY, SizeWidth, SizeHeight, SI <- Sprite' offset
;       Carry flag on -> Negetive and Overflow check
;       ImageFlipped -> Is the image should be drawn fliiped
;       Note: Flipped image and image checks ain't working together, you can only use one of of them
;Output:
;       None
;Registers:
;       AX, SI, DI, ES
;------------------------
PROC PrintSprite
        mov [SkipColumns], 0 ; Reset the amount of columns that needed to be skipped

        jnc @@SetStartPosition

;-----  Check if start point is out of bounds, if yes return
	cmp [StartX], ScreenWidth
	jl @@OutOfBounds
	ret ; If out of bounds, return

@@OutOfBounds:
        mov [ImageFlipped], False ; Reset image flipped

;-----  Check if the drawen sprite will not appear on the screen by adding to it's x value it's width
        mov ax, [StartX]
        add ax, [SizeWidth]
        cmp ax, 0
        jg @@NegX
        ret

@@NegX:
;-----  Check if we got a negetive x, if we did, we need to handle it
        cmp [StartX], 0
        jg @@CheckOverflow

@@FixNegX:
;-----  Get the absolute number of the amount of columns needed to be skipped
        mov ax, [StartX]
        xor ax, 0FFFFh
        sub ax, 0FFFFh

        mov [SkipColumns], ax ; Set the amount of columns needed to be skipepd

;-----  Remove from the width the wanted columns that needed to be skipped
        sub [SizeWidth], ax

        mov [StartX], 0 ; Reset the x value to 0

        add si, [SkipColumns] ; Skip the amount of columns needed from the start
        jmp @@SetStartPosition

@@CheckOverflow:
;-----  Check for overflow, that the sprite is drawing in the other side as well
        mov ax, [StartX]
        add ax, [SizeWidth]
        cmp ax, ScreenWidth
        jle @@SetStartPosition

@@FixOverflow:
;-----  Get the amount of columns that need to be skipped by subbing the screen size from the overflow amount
        sub ax, ScreenWidth
        mov [SkipColumns], ax

;-----  Get the new width of the sprite without the columns that are going to be skipped
        sub [SizeWidth], ax

@@SetStartPosition:
;-----  Set the starting position of printing the sprite
        mov ax, [StartY]
        mov [Y], ax

        mov ax, [StartX]
        mov [X], ax

        mov dx, [SizeWidth] ; Set width for line

        mov [Temp], 1 ; Set normal offset

;-----  If the image is flipped, reverse offset
        cmp [ImageFlipped], True
        jne @@Cycle

        mov [Temp], -1
        add si, [SizeWidth] ; Start from the end of the line
        dec si

@@Cycle:
        mov al, [si]
        mov [Color], al ; Get pixel' color and set in cl
        call PutPixel
        add si, [Temp] ; Point to next pixel' color by adding the offset that is saved in the temp byte var
        inc [X] ; Move pixel' x by 1

        dec dx ; Decrease width by 1
        jnz @@Cycle ; While we didn't reach 0 (End of line), keep on cycle

        cmp [ImageFlipped], True
        jne @@Normal

@@HandleFlipped:
        add si, [SizeWidth] ; Go back to end of current line
        add si, [SizeWidth] ; Start from the end of the new line

@@Normal:
        add si, [SkipColumns] ; Skip the amount of columns needed

        mov ax, [StartX]
        mov [X], ax ; Set ax to start of line again
        inc [Y] ; Point to a new line

        mov dx, [SizeWidth] ; Set width for line

        dec [SizeHeight] ; Decrease sprite' height
        jnz @@Cycle

        ret
ENDP PrintSprite

;------------------------
;PutPixel - Write dot
;------------------------
;Input:
;       X, Y, Color (All vars)
;Output:
;       None
;Registers:
;       AX, DI, ES
;------------------------
PROC PutPixel
        push ax
        mov di, [Y]
        mov ax, [Y]
        shl di, 8
        shl ax, 6
        add di, ax
        add di, [X]
        mov al, [Color]
        mov [es:di], al
        pop ax
        ret
ENDP PutPixel

;------------------------
;GetPixel - Read dot
;------------------------
;Input:
;       X, Y
;Output:
;       Color
;Registers:
;       AX, DI, ES
;------------------------
PROC GetPixel
        push ax
        mov di, [Y]
        mov ax, [Y]
        shl di, 8
        shl ax, 6
        add di, ax
        add di, [X]
        mov al, [es:di]
        mov [Color], al
        pop ax
        ret
ENDP GetPixel
