run: compile
	gvba program.gba
compile: pascals_adventure.c
	gbacc pascals_adventure.c
clean:
	rm program.gba
