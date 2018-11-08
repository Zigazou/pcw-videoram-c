#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "videoram.h"
#include "characters.h"

// Look up table: transforms 4 bits to 8 bits by duplicating each bit.
// Ex.: 1001 -> 11000011
static unsigned char double_bits_full[16] = {
    0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255
};

// Look up table: transforms 4 bits to 8 bits by inserting a bit 0 between each
// bit.
// Ex.: 1111 -> 10101010
static unsigned char double_bits_half[16] = {
    0, 2, 8, 10, 32, 34, 40, 42, 128, 130, 136, 138, 160, 162, 168, 170
};

// The video structure contains global variables for this library.
static struct {
    unsigned int *roller;       // Roller RAM address
    unsigned int *line_starts;  // Line start offsets
    unsigned char *screen;      // Screen memory address
    unsigned char row;          // Row where the next character will be printed
    unsigned char col;          // Column where the next character will be
                                // printed
    unsigned char *address;     // Screen address where the next character will
                                // be printed
    unsigned char *font;        // Address of the character font
    unsigned char font_size;    // Size at which the next character will be
                                // printed
    unsigned char *brightness;  // Brightness for double width character
} video = { NULL, NULL, 0, 0, NULL, NULL, 0, NULL };

// Function type for a print function
typedef void PRINT_FUNCTION(const char *string);

// Declare the four print functions
void print_normal_size(const unsigned char *string);
void print_double_width(const unsigned char *string);
void print_double_height(const unsigned char *string);
void print_double_size(const unsigned char *string);

// Pointers to the print functions
PRINT_FUNCTION *prints[] = {
    print_normal_size,
    print_double_width,
    print_double_height,
    print_double_size
};

// Set the character size.
// The available values are SIZE_NORMAL, SIZE_DOUBLE_WIDTH, SIZE_DOUBLE_HEIGHT,
// and SIZE_DOUBLE.
void set_size(unsigned char size) {
    video.font_size = size;
}

// Set brightness for double width characters.
// The available values are BRIGHTNESS_FULL and BRIGHTNESS_HALF.
void set_brightness(unsigned char brightness) {
    if(brightness == BRIGHTNESS_HALF) {
        video.brightness = double_bits_half;
    } else {
        video.brightness = double_bits_full;
    }
}

// Allocate memory for screen and roller RAM.
// The stack is placed by the C runtime at the top of free memory. The screen
// memory is placed right under the stack. The stack_size parameter sets the
// size of the stack to keep available.
void alloc_screen_memory(unsigned int stack_size) {
    // Trick to get the stack address.
    void *p = NULL;

    // The roller RAM address must be a multiple of 512.
    video.roller = (&p - stack_size - SCREEN_SIZE - ROLLER_SIZE * 2) & 0xFE00;
    video.line_starts = (&p - stack_size - SCREEN_SIZE - ROLLER_SIZE) & 0xFE00;

    // Video memory directly follows the roller RAM.
    video.screen = video.roller + ROLLER_ENTRIES * 2;
}

// Initialize the roller RAM to point at our own screen memory
void init_roller_ram() {
    unsigned char line;
    unsigned char row;
    unsigned int index;
    unsigned int address;
    unsigned int inbank;

    // The roller RAM has one entry for each screen line
    index = 0;
    address = (unsigned int)video.screen;

    // There are 32 rows on a PCW screen
    for(row = 0; row < 32; row++) {
        // Each row groups 8 screen lines
        for(line = 0; line < 8; line++) {
            // Determines which RAM bank will hold the line
            inbank = address & (BANK_SIZE - 1);

            video.line_starts[index] = address;
            video.roller[index++] = (((address >> 14) + BASE_BANK) << 13)
                                  + ((inbank >> 1) & 0xFFF8)
                                  + (inbank & 7);

            address++;
        }

        address += 720-8;
    }
}

// Sets the roller RAM address
void set_roller_ram_address() {
    unsigned int bank;
    unsigned int address;

    // Determines which RAM bank will hold the roller RAM
    bank = BASE_BANK + ((unsigned int)video.roller >> 14);

    // Determines the address of the roller RAM in this bank
    address = (unsigned int)video.roller & (BANK_SIZE - 1);

    outp(SET_ROLLER_ADDRESS, (bank * 32) + (address >> 9));
}

// Change back the roller RAM to its standard address
void restore_video_ram() {
    outp(SET_ROLLER_ADDRESS, 0x5B);
}

// Clear the screen
void clear_screen() {
    memset(video.screen, 0, SCREEN_SIZE);
}

// Sets the position of the next character to be printed.
// col=[0..89], row=[0..31]
void locate(unsigned char col, unsigned char row) {
    video.row = row;
    video.col = col;
    video.address = video.screen + row * 720 + col * 8;
}

// Update the cursor position after each printed character.
void advance_cursor() {
    if(video.font_size & 1) {
        video.col += 2;
        video.address += 16;
    } else {
        video.col++;
        video.address += 8;
    }

    if(video.col < 90) return;

    video.col = 0;

    if(video.font_size & 2) {
        video.row += 2;
        video.address += 720;
    } else {
        video.row++;
    }

    if(video.row < 32) return;

    video.row = 0;
    video.address = video.screen;
}

// Main print function which uses the dedicated print function given the current
// settings.
void print(const unsigned char *string) {
    prints[video.font_size](string);
}

// Print normal size characters. This uses memcpy in order to draw characters
// at the maximum speed.
void print_normal_size(const unsigned char *string) {
    unsigned char i;

    for(; *string != '\0'; string++) {
        memcpy(video.address, &video.font[*string * 8], 8);
        advance_cursor();
    }
}

// Print double width characters.
void print_double_width(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string * 8];
        for(i = 0; i != 8; i++) {
            // Left part
            video.address[i] = video.brightness[*character_drawing >> 4];

            // Right part
            video.address[i+8] = video.brightness[*character_drawing & 15];

            character_drawing++;
        }
        advance_cursor();
    }
}


// Look up table to accelerate character drawing when using double height.
const unsigned int dh_offset[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 720, 721, 722, 723, 724, 725, 726, 727
};

// Print double height characters.
void print_double_height(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string * 8];
        for(i = 0; i != 16; i+= 2) {
            // The same line is printed twice
            video.address[dh_offset[i]] = *character_drawing;
            video.address[dh_offset[i+1]] = *character_drawing;

            character_drawing++;
        }

        advance_cursor();
    }
}

// Print double size characters.
void print_double_size(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;
    unsigned char left;
    unsigned char right;
    unsigned int offset;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string * 8];
        for(i = 0; i != 16; i+= 2) {
            left = video.brightness[*character_drawing >> 4];
            right = video.brightness[*character_drawing & 15];
            offset = dh_offset[i];

            // Upper left part
            video.address[offset] = left;

            // Upper right part
            video.address[offset+8] = right;

            // Bottom left part
            video.address[offset+1] = left;

            // Bottom right part
            video.address[offset+9] = right;

            character_drawing++;
        }

        advance_cursor();
    }
}

// Defines which font to use when printing characters on the screen.
void set_font(unsigned char *font) {
    video.font = font;
}

unsigned char vertical_masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
void vertical_line(unsigned int x, unsigned char y1, unsigned char y2) {
    unsigned char mask;
    unsigned char y;
    unsigned int offset;
    unsigned char *address;

    mask = vertical_masks[x & 7];
    offset = x & 0xfff8;

    for(y = y1; y <= y2; y++) {
        address = (unsigned char *)(video.line_starts[y]) + offset;
        *address |= mask;
    }
}

// Initializes everything!
void init_video_ram(unsigned int stack_size) {
    alloc_screen_memory(stack_size);
    locate(0, 0);
    set_size(SIZE_NORMAL);
    set_brightness(BRIGHTNESS_FULL);
    set_font(stdfont);
    init_roller_ram();
    clear_screen();
    set_roller_ram_address();
}
