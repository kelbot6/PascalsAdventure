@ pan_collide.s

/* function to check if pascal and the pan have collided */
.global pan_collide
pan_collide:

    @shift our params over to make room for r0 return: [r2 = pascal->y] and [r1 = pan->y]
	mov r2, r1
	mov r1, r0

	@r3 is a temp of pan->y. Add 8 to it (lower bound of pan) and check its bounds with pascal's upper bound
	mov r3, r1
	add r3, r3, #8
    cmp r3, r2
    bgt .next

	@if we make it here, they have not collided
	mov r0, #0
	b .end

	.next:
		@we already have r3 set to the pan's lower bound. Compare this with pascal->y + 8 (pascal's lower bound)
		add r2, r2, #8
		cmp r3, r2
		blt .yesCollide

		@if we make it here, they have not collided
		mov r0, #0
		b .end

	.yesCollide:
		mov r0, #1
		b .end

	.end:
		@return
		mov pc, lr
