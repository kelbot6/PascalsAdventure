/*
 * pascals_adventure.c
 * He's a boss
 */

/* include the image palettes we are using */
#include "tower_tiles.h"
#include "sprite_sheet.h"
#include "letter_tiles.h"

/* include the tile maps we are using */
#include "background1.h"
#include "background2.h"
#include "background3.h"
#include "background4.h"
#include "background_text.h"

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

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000

/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memeory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palette */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;

volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

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

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
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
    *bg0_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (28 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

    /* load the tile data into screen block 16 through 19 */
    dest = screen_block(28);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background1[i*64 + j];
        }
    }   
    dest = screen_block(29);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background1[i*64+32 + j];
        }
    }   
    dest = screen_block(30); 
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background1[i*64 + j + 2048];
        }
    }
    dest = screen_block(31); 
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background1[i*64+32 + j + 2048];
        }
    }

	//*******************************************************
    /* load the image into char block 0 (16 bits at a time) */
    dest = char_block(0);
    image = (unsigned short*) tower_tiles_data;
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

/* display either the rules, losing, or winning screen */
void display_text() {
	/* load the palette from the image into palette memory */
	for (int i = 0; i < PALETTE_SIZE; i++){	
		bg_palette[i] = letter_tiles_palette[i];
	}
	//******************************************************
	/* load the image into char block 0 (16 bits at a time) */
	volatile unsigned short* dest = char_block(0);
	unsigned short* image = (unsigned short*) letter_tiles_data;
	for (int i = 0; i < ((letter_tiles_width * letter_tiles_height) / 2); i++){
		dest[i] = image[i];
	}
	
	/* set all control the bits in this register */
    *bg0_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (28 << 8) |       /* the screen block the tile data is stored in */
        (0 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */

	/* load the tile data into screen block 16 through 19 */
    dest = screen_block(28);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background_text[i*64 + j];
        }
    }   
    dest = screen_block(29);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background_text[i*64+32 + j];
        }
    }   
    dest = screen_block(30); 
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background_text[i*64 + j + 2048];
        }
    }
    dest = screen_block(31); 
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            dest[i*32 + j] = background_text[i*64+32 + j + 2048];
        }
    }
}

/* just kill time */
void delay(unsigned int amount); /*{
    for (int i = 0; i < amount * 10; i++);
}*/

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /* rendering mode */
                            (0 << 10) |         /* gfx mode */
                            (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode, 0:16, 1:256 */
                            (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = HEIGHT;
        sprites[i].attribute1 = WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) sprite_sheet_palette, PALETTE_SIZE);

    /* load the image into sprite image memory */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) sprite_sheet_data, (sprite_sheet_width * sprite_sheet_height) / 2);
}

/* a struct to handle all possible sprites' behavior */
struct Thing {

    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion */
    float x, y;

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the pascal is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the pascal stays */
    int border;
};

/***************************************************************************************/
/* initialize Pascal */
void pascal_init(struct Thing* pascal) {

	pascal->x = 128;
	pascal->y = 130;
	pascal->border = 80;
	pascal->frame = 16;
	pascal->move = 0;
	pascal->counter = 0;
	pascal->animation_delay = 16;
	pascal->sprite = sprite_init(pascal->x, pascal->y, SIZE_8_8, 0, 0, pascal->frame, 0);
}

int pascal_up(struct Thing* pascal) {
    /* face up */
    //sprite_set_vertical_flip(pascal->sprite, 0);
    pascal->move = 1;

    /* if we are at the top end, just scroll the screen */
    if (pascal->y < (HEIGHT - pascal->border)) {
        return 1;
    } else {
        /* else move up */
        pascal->y--;
        return 0;
    }
}

/* stop */
void pascal_stop(struct Thing* pascal) {

	pascal->move = 0;
	pascal->frame = 16;
	pascal->counter = 7;
	sprite_set_offset(pascal->sprite, pascal->frame);
}

/* update the thing */
void pascal_update(struct Thing* pascal) {

	//Flip between frames 12 and 20 (frame 16 is stationary pascal)
	if(pascal->move == 1) {

		pascal->counter++;
		if(pascal->counter >= pascal->animation_delay) {

			if(pascal->frame == 12) {

				pascal->frame = 20;
			}
			else {

				pascal->frame = 12;
			}
			sprite_set_offset(pascal->sprite, pascal->frame);
			pascal->counter = 0;

		}
	}
	else if(pascal->move == 2) {

		pascal->frame = 28;
		sprite_set_offset(pascal->sprite, pascal->frame);
	}
	sprite_position(pascal->sprite, pascal->x, pascal->y);
}

/*****************************************************************************************/
/* initialize Bird */
void bird_init(struct Thing* bird) {

    bird->x = 8;
    bird->y = 80;
    bird->border = 40;
    bird->frame = 14;
    bird->move = 1;
    bird->counter = 0;
    bird->animation_delay = 24;
    bird->sprite = sprite_init(bird->x, bird->y, SIZE_8_8, 0, 0, bird->frame, 0);
}

/* stop */
void bird_stop(struct Thing* bird) {

    bird->move = 0;
    bird->frame = 14;
    bird->counter = 7;
    sprite_set_offset(bird->sprite, bird->frame);
}

/* update the thing */
void bird_update(struct Thing* bird, struct Thing* pascal, int birdVelocity, int* birdCounter) {

	int birdDelay = 50;
	*birdCounter++;

    if(bird->move) {

		//bird->y = pascal->y;

		if(birdVelocity > 0 && *birdCounter >= birdDelay) {

			if(bird->x < pascal->x) {

				bird->y = bird->y + 0.20;
			}
			else if(bird->x > pascal->x) {

				bird->y = bird->y - 0.20;
			}

			sprite_set_horizontal_flip(bird->sprite, 0);
			bird->x+=.50;
			birdCounter = 0;
		}
		else if(birdVelocity < 0 && *birdCounter >= birdDelay) {

            if(bird->x > pascal->x) {

                bird->y = bird->y + 0.20;
            }
            else if(bird->x < pascal->x) {

                bird->y = bird->y - 0.20;
            }

			sprite_set_horizontal_flip(bird->sprite, 1);
			bird->x-=.50;
			birdCounter = 0;
		}

		if(pascal->move == 1 && !(pascal->y <= 80)) {

			bird->y = bird->y - 1.00;
		}

		//frames 14, 18, 22
        bird->counter++;
        if(bird->counter >= bird->animation_delay) {

            bird->frame = bird->frame + 4;
            if(bird->frame > 22) {

                bird->frame = 14;
            }
            sprite_set_offset(bird->sprite, bird->frame);
            bird->counter = 0;

        }
    }
    sprite_position(bird->sprite, bird->x, bird->y);
}

/*****************************************************************************************/
/* initialize Pan */
void pan_init(struct Thing* pan) {

    pan->x = 128;
    pan->y = 0;
    pan->border = 40;
    pan->frame = 24;
    pan->move = 1;
    pan->counter = 0;
    pan->animation_delay = 16;
    pan->sprite = sprite_init(pan->x, pan->y, SIZE_8_8, 0, 0, pan->frame, 0);
}

/* stop */
void pan_stop(struct Thing* pan) {

    pan->move = 0;
    pan->frame = 24;
    pan->counter = 7;
    sprite_set_offset(pan->sprite, pan->frame);
}

/* update the thing */
void pan_update(struct Thing* pan, struct Thing* pascal) {

    if(pan->move) {

        pan->counter++;
        if(pan->counter >= pan->animation_delay) {

            pan->frame = pan->frame + 2;
            if(pan->frame > 26) {

                pan->frame = 24;
            }
            sprite_set_offset(pan->sprite, pan->frame);
            pan->counter = 0;
			if(pascal->move == 1) {

				pan->y+=12;
			}
			else {

				pan->y+=6;
			}
			if(pan->y + 8 >= HEIGHT) {

				pan->y = 0;
			}
        }
    }
    sprite_position(pan->sprite, pan->x, pan->y);
}

/* import our assembly functions to help with collision checking */
int pan_collide(int pan_y, int pascal_y);
int bird_collide_right(int bird_velocity, int pascal_x, int bird_x);

/* Check for collision */
int collision(struct Thing* pascal, struct Thing* pan, struct Thing* bird, int birdVelocity) {

	if(pascal->move != 2) {

		/* did pascal and pan collide */
		/*if(pan->y + 8 > pascal->y && pan->y + 8 <  pascal->y + 8) {

			return 1;
		}*/
		if(pan_collide((int) pan->y, (int)  pascal->y) == 1) {

			return 1;
		}
		/* did pascal and pan collide */
		/*else if(birdVelocity > 0 && pascal->x == bird->x + 8) {

			return 1;
		}*/
		else if(bird_collide_right(birdVelocity, (int) pascal->x, (int) bird->x) == 1) {

			return 1;
		}
		else if(birdVelocity < 0 && pascal->x + 8 == bird->x) {

			return 1;
		}
	}
	return 0;

}

/* the main function */
int main() {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | BG2_ENABLE | BG3_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

	/* create Bird */
	struct Thing bird;
	bird_init(&bird);
	//Give the bird an initial velocity and delay counter
	int birdVelocity = 1;
	int birdCounter = 0;

	/* create Pan */
	struct Thing pan;
	pan_init(&pan);

    /* create pascal */
    struct Thing pascal;
    pascal_init(&pascal);

    /* set initial scroll to 0 */
    int xscroll = 0;
    int yscroll = 0;
	int yscroll_text = 0;

	display_text();
	*bg0_y_scroll = 165 + yscroll_text;
	delay(300500);

	setup_background();

    /* loop forever */
    while (1) {

		if(yscroll <= -300) {
			wait_vblank();
			// display winning screen
			display_text();
			*bg0_y_scroll = yscroll_text;
			delay(100500);
			break;
		}

		if(collision(&pascal, &pan, &bird, birdVelocity) == 1) {
			wait_vblank();
			// display losing screen
			display_text();
			*bg0_y_scroll = 350 + yscroll_text;
			delay(100500);
			break;
		}

		if(bird.x >= WIDTH - 8) {

			birdVelocity = -1;
		}
		else if(bird.x <= 0) {

			birdVelocity = 1;
		}

		/* update pascal */
		pascal_update(&pascal);
		bird_update(&bird, &pascal, birdVelocity, &birdCounter);
		pan_update(&pan, &pascal);

        /* scroll with the arrow keys */
        if (button_pressed(BUTTON_DOWN)) {
            //yscroll++;
        }
        else if (button_pressed(BUTTON_UP) && pascal.move != 2) {

			if(pascal_up(&pascal)) {

				yscroll--;
			}

        }
        else if (button_pressed(BUTTON_RIGHT)) {
            //xscroll++;
        }
        else if (button_pressed(BUTTON_LEFT)) {
            //xscroll--;
        }
		else if (button_pressed(BUTTON_A)) {

			pascal.move = 2;
		}
		else {

			pascal_stop(&pascal);
			//bird_stop(&bird);
			//pan_stop(&pan);
		}

        /* wait for vblank before scrolling */
        wait_vblank();
        //*bg0_x_scroll = xscroll;
        *bg0_y_scroll = 350 + yscroll;

		//*bg1_x_scroll = xscroll;
		*bg1_y_scroll = 350 + yscroll;

		xscroll--;
		*bg2_x_scroll = xscroll / 20;
		*bg2_y_scroll = (yscroll / 2) - 200;

		//*bg3_x_scroll = xscroll;
		*bg3_y_scroll = 355 + yscroll / 4;

		sprite_update_all();

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


