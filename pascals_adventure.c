/*
 * pascals_adventure.c
 * He's a boss
 */

/* include the image palettes we are using */
#include "tower_tiles.h"

/* include the tile maps we are using */
#include "background2.h"
#include "background3.h"
#include "background4.h"

/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* the three tile modes */
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

/* enable bits for the four tile layers */
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;


/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}


/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}


/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}


/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    for (int i = 0; i < PALETTE_SIZE; i++) {
        bg_palette[i] = tower_tiles_palette[i];
    }

	//*******************************************************
    /* load the image into char block 0 (16 bits at a time) */
    volatile unsigned short* dest = char_block(0);
    unsigned short* image = (unsigned short*) tower_tiles_data;
    for (int i = 0; i < ((tower_tiles_width * tower_tiles_height) / 2); i++) {
        dest[i] = image[i];
    }

    /* set all control the bits in this register */
    *bg1_control = 1 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (8 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

    /* load the tile data into screen block 16 through 19 */
    dest = screen_block(8);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background2[i*64 + j];
        }
    }
    dest = screen_block(9);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background2[i*64+32 + j];
        }
    }
    dest = screen_block(10);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background2[i*64 + j + 2048];
        }
    }
    dest = screen_block(11);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background2[i*64+32 + j + 2048];
        }
    }

	//*******************************************************
	/* load the image into char block 0 (16 bits at a time) */
    /*dest = char_block(0);
	image = (unsigned short*) tower_tiles_data;
    for (int i = 0; i < ((tower_tiles_width * tower_tiles_height) / 2); i++) {
        dest[i] = image[i];
    }*/

	/* set all control the bits in this register */
    *bg2_control = 2 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (16 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

	/* load th data into screen block 16 through 19 */
    dest = screen_block(16);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background3[i*64 + j];
        }
    }
    dest = screen_block(17);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background3[i*64+32 + j];
        }
    }
    dest = screen_block(18);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background3[i*64 + j + 2048];
        }
    }
    dest = screen_block(19);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background3[i*64+32 + j + 2048];
        }
    }

    //*******************************************************
    /* load the image into char block 0 (16 bits at a time) */
    /*dest = char_block(0);
	unsigned short* image2 = (unsigned short*) mountain_tiles_data;
    for (int i = 0; i < ((mountain_tiles_width * mountain_tiles_height) / 2); i++) {
        dest[i] = image2[i];
    }*/

    /* set all control the bits in this register */
    *bg3_control = 3 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (24 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

    /* load the tile data into screen block 16 through 19 */
    dest = screen_block(24);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background4[i*64 + j];
        }
    }
    dest = screen_block(25);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background4[i*64+32 + j];
        }
    }
    dest = screen_block(26);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background4[i*64 + j + 2048];
        }
    }
    dest = screen_block(27);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background4[i*64+32 + j + 2048];
        }
    }
}

/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* the main function */
int main() {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG1_ENABLE | BG2_ENABLE | BG3_ENABLE;

    /* setup the background 0 */
    setup_background();

    /* set initial scroll to 0 */
    int xscroll = 0;
    int yscroll = 0;

    /* loop forever */
    while (1) {
        /* scroll with the arrow keys */
        if (button_pressed(BUTTON_DOWN)) {
            yscroll++;
        }
        if (button_pressed(BUTTON_UP)) {
            yscroll--;

        }
        if (button_pressed(BUTTON_RIGHT)) {
            //xscroll++;
        }
        if (button_pressed(BUTTON_LEFT)) {
            //xscroll--;
        }

        /* wait for vblank before scrolling */
        wait_vblank();
        //*bg0_x_scroll = xscroll / 2;
        //*bg0_y_scroll = yscroll / 2;

		//*bg1_x_scroll = xscroll;
		*bg1_y_scroll = 350 + yscroll;

		xscroll--;
		*bg2_x_scroll = xscroll / 20;
		*bg2_y_scroll = yscroll / 2;

		//*bg3_x_scroll = xscroll;
		*bg3_y_scroll = 355 + yscroll / 4;

        /* delay some */
        delay(50);
    }
}


/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore() {
    /* do nothing */
}


/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)();
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};


