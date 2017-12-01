@ bird_collide_right.s

/* function to check if pascal and the bird have collided */
.global bird_collide_right
bird_collide_right:

    @shift our params over to make room for r0 return: [r3 = bird->x], [r2 = pascal->x] and [r1 = birdVelocity]
	mov r3, r2
    mov r2, r1
    mov r1, r0

    @Check if the velocity is positive (bird moving left to right)
    cmp r1, #0
    bgt .next

    @if we make it here, they have not collided
    mov r0, #0
    b .end

    .next:
        @Compare pascal->x (pascal's left bound) with bird->x + 8 (bird's right bound)
        add r3, r3, #8
        cmp r3, r2
        beq .yesCollide

        @if we make it here, they have not collided
        mov r0, #0
        b .end

    .yesCollide:
        mov r0, #1
        b .end

    .end:
        @return
        mov pc, lr
