run: compile
	gvba program.gba
compile: pan_collide.s bird_collide_right.s delay.s pascals_adventure.c
	gbacc pan_collide.s bird_collide_right.s delay.s pascals_adventure.c
clean:
	rm program.gba
