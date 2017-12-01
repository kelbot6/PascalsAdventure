@ delay.s

/* kills some time */
.global delay

delay:

	mov r1, #10
	mul r0, r1, r0

	.top:
		cmp r0, #0
		beq .end

		sub r0, r0, #1
		b .top

	.end:
		mov pc, lr
