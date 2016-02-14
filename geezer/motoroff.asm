; Snippet to turn off floppy disk motors

	mov dx,3F2h
	mov al,0
	out dx,al
