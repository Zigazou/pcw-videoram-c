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
/*
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
*/
#asm
    ; IX = stack frame
    ; string +4
    ; i -1, character_drawing -3, left -4, right -5, offset -7
    push ix
    ld ix, 0
    add ix, sp

    push bc
    push bc
    push bc
    dec sp

    ld l, (ix+4)
    ld h, (ix+5)

.forloop_pds
    ; for(; *string != '\0'; string++) {
    ld a,(hl)
    or a
    jp z, endloop_pds

    inc hl
    push hl

    ; *string * 8
    ld l, a
    ld h, 0
    add hl, hl
    add hl, hl
    add hl, hl

    ; character_drawing = &video.font[*string * 8];
    ex de, hl
    ld hl, (_video+10)
    add hl, de
    ex de, hl ; de = &video.font[*string * 8]

    ld a, 0
    ; for(i = 0; i != 16; i+= 2) {
.forloop_pds_i
    cp 16
    jp c, forloop_pds_i_inside
    push de
    call _advance_cursor
    pop de
    pop hl
    jp forloop_pds

.forloop_pds_i_inside
    push af

    ; offset = dh_offset[i];
    ld hl, _dh_offset
    add a
    ld c, a
    ld b, 0
    add hl, bc
    ld c, (hl)
    inc hl
    ld b, (hl)
    push bc
    pop iy
    ld bc, (_video+8)
    add iy, bc ; iy = &video.address[offset]

    ; *character_drawing >> 4
    ld a, (de)
    rra
    rra
    rra
    rra
    and 0x0f

    ; left = video.brightness[*character_drawing >> 4];
    ld hl, (_video+13)
    add l
    ld l, a
    jr nc, ASMPC+3
    inc h
    ld a, (hl)

    ; video.address[offset] = left; // a = left, iy = offset
    ; video.address[offset+1] = left;
    ld (iy+0), a
    ld (iy+1), a

    ; *character_drawing & 0x0f
    ld a, (de)
    and 0x0f

    ; right = video.brightness[*character_drawing & 15];
    ld hl, (_video+13)
    add l
    ld l, a
    jr nc, ASMPC+3
    inc h
    ld a, (hl)

    ; video.address[offset+8] = right; // a = right
    ; video.address[offset+9] = right;
    ld (iy+8), a
    ld (iy+9), a

    pop af
    add a, 2
    inc de
    jp forloop_pds_i

.endloop_pds
    inc sp
    pop bc
    pop bc
    pop bc
    pop ix
    ret
#endasm
}

// Defines which font to use when printing characters on the screen.
void set_font(unsigned char *font) {
    video.font = font;
}

unsigned char vertical_masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
void vertical_line(unsigned int x, unsigned char y1, unsigned char y2) {
/*    unsigned char mask;
    unsigned char y;
    unsigned int offset;
    unsigned char *address;
    unsigned int *line_start;

    mask = vertical_masks[(unsigned char)x & 7];
    offset = x & 0xfff8;

    line_start = &video.line_starts[y1];
    for(y = y1; y != y2 + 1; y++) {
        address = (unsigned char *)(*line_start) + offset;
        *address |= mask;
        line_start++;
    }
*/
#asm
    ; x +8, y1 +6, y2 +4
    ; mask -1, y -2, offset -4, line_start -6
    ; IX = stack frame
    push ix
    ld ix, 0
    add ix, sp

    ; Reserve 6 bytes on the stack
    push bc
    push bc
    push bc

    ; mask = vertical_masks[(unsigned char)x & 7];
    ld a, (ix+8) ; x
    and 7
    ld e, a
    ld d, 0
    ld hl, _vertical_masks
    add hl,de
    ld a,(hl)

    ld (ix-1), a ; mask

    ; offset = x & 0xfff8;
    ld a, (ix+8) ; x
    and 0xf8
    ld e, a
    ld d, (ix+9)

    ld (ix-4), e ; offset
    ld (ix-3), d

    ; line_start = &video.line_starts[y1];
    ld e, (ix+6) ; y1
    ld d, 0

    ld    hl, (_video+1+1)
    add hl, de
    add hl, de

    ex (sp), hl

    ; for(y = y1; y != y2 + 1; y++) {
    ld a, (ix+4) ; y2
    sub a, (ix+6) ; y1

    or a
    jp z, endfor

    pop de
    push de

.forloop
    ld l, e
    ld h, d

    ; address = (unsigned char *)(*line_start) + offset;
    ld c,(hl)
    inc hl
    ld b,(hl) ; bc = *line_start

    ld l, (ix-4) ; hl=offset
    ld h, (ix-3)

    add hl, bc ; hl = *line_start + offset

    ; *address |= mask;
    ld c, (ix-1) ; mask

    ld b, a
    ld a, (hl)
    or c
    ld (hl), a
    ld a, b

    ; line_start++;
    inc de
    inc de

    dec a
    jp nz, forloop

.endfor

    pop bc
    pop bc
    pop bc
    pop ix
    ret
#endasm
}

unsigned char horz_start_masks[] = { 255, 127, 63, 31, 15, 7, 3, 1 };
unsigned char horz_end_masks[] = { 128, 192, 224, 240, 248, 252, 254, 255 };
void horizontal_line(unsigned int x1, unsigned int x2, unsigned char y) {
    unsigned char start_mask;
    unsigned char end_mask;
    unsigned char pixel_count;
    unsigned char i;
    unsigned char *address;

    start_mask = horz_start_masks[(unsigned char)x1 & 7];
    end_mask = horz_end_masks[(unsigned char)x2 & 7];

    address = (unsigned char *)(video.line_starts[y]) + (x1 & 0xfff8);
    if(x1 & 0xfff8 == x2 & 0xfff8) {
        *address |= start_mask & end_mask;
        return;
    }

    *address |= start_mask;
    address += 8;

    pixel_count = (((x2 & 0xfff8) - (x1 & 0xfff8)) >> 3) - 1;
    for(i = 0; i != pixel_count; i++) {
        *address = 255;
        address += 8;
    }

    *address |= end_mask;
}

void frame(unsigned int tx, unsigned char ty, unsigned int bx, unsigned char by) {
    vertical_line(tx, ty, by);
    vertical_line(bx, ty, by);
    horizontal_line(tx, bx, ty);
    horizontal_line(tx, bx, by);
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
