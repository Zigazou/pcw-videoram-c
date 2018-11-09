#ifndef VIDEORAM_H
#define VIDEORAM_H

#define ROLLER_ENTRIES 256
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 256
#define SET_ROLLER_ADDRESS 0xF5

#define SCREEN_SIZE 23040
#define ROLLER_SIZE 512
#define FONT_SIZE 2048
#define BANK_SIZE 16384
#define BASE_BANK 4

#define SIZE_NORMAL 0
#define SIZE_DOUBLE_WIDTH 1
#define SIZE_DOUBLE_HEIGHT 2
#define SIZE_DOUBLE 3

#define BRIGHTNESS_FULL 0
#define BRIGHTNESS_HALF 1

extern void init_video_ram(unsigned int stack_size);
extern void set_font(unsigned char *font);
extern void set_size(unsigned char size);
extern void set_brightness(unsigned char brightness);
extern void restore_video_ram();
extern void clear_screen();
extern void locate(unsigned char col, unsigned char row);
extern void print(const unsigned char *string);
extern void vertical_line(unsigned int x, unsigned char y1, unsigned char y2);
extern void horizontal_line(unsigned int x1, unsigned int x2, unsigned char y);
extern void frame(unsigned int tx, unsigned char ty, unsigned int bx, unsigned char by);

#endif
