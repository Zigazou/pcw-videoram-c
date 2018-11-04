#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "videoram.h"
#include "characters.h"

// Transforms 4 bits to 8 bits by duplicating each bit.
// Ex.: 1001 -> 11000011
static unsigned int double_bits_full[16] = {
    0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255
};

// Transforms 4 bits to 8 bits by inserting a bit 0 between each bit.
// Ex.: 1111 -> 10101010
static unsigned int double_bits_half[16] = {
    0, 2, 8, 10, 32, 34, 40, 42, 128, 130, 136, 138, 160, 162, 168, 170
};

static struct {
    unsigned int *roller;
    unsigned char *screen;
    unsigned char row;
    unsigned char col;
    unsigned char *address;
    unsigned char *font;
    unsigned char font_size;
    unsigned int *brightness;
} video = { NULL, NULL, 0, 0, NULL, NULL, 0, NULL };

typedef void PRINT_FUNCTION(const char *string);

void print_normal_size(const unsigned char *string);
void print_double_width(const unsigned char *string);
void print_double_height(const unsigned char *string);
void print_double_size(const unsigned char *string);

PRINT_FUNCTION *prints[] = {
    print_normal_size,
    print_double_width,
    print_double_height,
    print_double_size
};

void set_size(unsigned char size) {
    video.font_size = size;
}

void set_brightness(unsigned char brightness) {
    if(brightness == BRIGHTNESS_HALF) {
        video.brightness = double_bits_half;
    } else {
        video.brightness = double_bits_full;
    }
}

void alloc_screen_memory(unsigned int stack_size) {
    // Trick to get the stack address.
    void *p = NULL;

    // The roller RAM address must be a multiple of 512.
    video.roller = (&p - stack_size - SCREEN_SIZE - ROLLER_SIZE) & 0xFE00;

    // Video memory directly follows the roller RAM.
    video.screen = video.roller + ROLLER_ENTRIES;
}

void init_roller_ram() {
    unsigned char line;
    unsigned char row;
    unsigned int index;
    unsigned int address;
    unsigned int inbank;

    // The roller RAM has one entry for each screen line.
    index = 0;
    address = (unsigned int)video.screen;
    for(row = 0; row < 32; row++) {
        for(line = 0; line < 8; line++) {
            inbank = address & (BANK_SIZE - 1);
            video.roller[index++] = (((address >> 14) + BASE_BANK) << 13)
                                  + ((inbank >> 1) & 0xFFF8)
                                  + (inbank & 7);
            address++;
        }

        address += 720-8;
    }
}

void set_roller_ram_address() {
    unsigned int bank;
    unsigned int address;

    bank = BASE_BANK + ((unsigned int)video.roller >> 14);
    address = (unsigned int)video.roller & (BANK_SIZE - 1);
    outp(SET_ROLLER_ADDRESS, (bank << 5) + (address >> 9));
}

void restore_video_ram() {
    outp(SET_ROLLER_ADDRESS, 0x5B);
}

void clear_screen() {
    memset(video.screen, 0, SCREEN_SIZE);
}

void locate(unsigned char col, unsigned char row) {
    video.row = row;
    video.col = col;
    video.address = video.screen + row * 720 + col * 8;
}

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

void print(const unsigned char *string) {
    prints[video.font_size](string);
}

void print_normal_size(const unsigned char *string) {
    unsigned char i;

    for(; *string != '\0'; string++) {
        memcpy(video.address, &video.font[*string << 3], 8);
        advance_cursor();
    }
}

void print_double_width(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string << 3];
        for(i = 0; i < 8; i++) {
            video.address[i] = video.brightness[*character_drawing >> 4];
            video.address[i+8] = video.brightness[*character_drawing & 15];
            character_drawing++;
        }
        advance_cursor();
    }
}

const unsigned int dh_offset[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 720, 721, 722, 723, 724, 725, 726, 727
};

void print_double_height(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string << 3];
        for(i = 0; i < 16; i+= 2) {
            video.address[dh_offset[i]] = *character_drawing;
            video.address[dh_offset[i+1]] = *character_drawing;
            character_drawing++;
        }

        advance_cursor();
    }
}

void print_double_size(const unsigned char *string) {
    unsigned char i;
    unsigned char *character_drawing;

    for(; *string != '\0'; string++) {
        character_drawing = &video.font[*string << 3];
        for(i = 0; i < 16; i+= 2) {
            video.address[dh_offset[i]] = video.brightness[*character_drawing >> 4];
            video.address[dh_offset[i]+8] = video.brightness[*character_drawing & 15];
            video.address[dh_offset[i]+1] = video.brightness[*character_drawing >> 4];
            video.address[dh_offset[i]+9] = video.brightness[*character_drawing & 15];
            character_drawing++;
        }

        advance_cursor();
    }
}

void set_font(unsigned char *font) {
    video.font = font;
}

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
