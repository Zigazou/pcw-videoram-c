#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "videoram.h"

#pragma output noprotectmsdos
#pragma output noredir
#pragma output nogfxglobals

// A 2048 bytes stack should be more than enough for this little demo
#define STACK_SIZE 2048

// Line that will receive characters
static unsigned char line[] = "                                ";

main() {
    unsigned char i;
    unsigned char j;
    unsigned int x;
    unsigned char y;

    init_video_ram(STACK_SIZE);

    // Print the title frame
    set_size(SIZE_DOUBLE_WIDTH);
    locate(3, 0);
    print("\226\232\232\232\232\232\232\232\234");
    set_size(SIZE_DOUBLE);
    locate(3, 1);
    print("\225       \225");
    set_size(SIZE_DOUBLE_WIDTH);
    locate(3, 3);
    print("\223\232\232\232\232\232\232\232\231");

    // Print the title
    set_size(SIZE_DOUBLE_HEIGHT);
    locate(5, 1);
    print("VideoRAM demo!");

    // Print explanations in french
    set_size(SIZE_NORMAL);
    locate(0, 5);
    print("Le jeu de caract}res de");
    locate(0, 6);
    print("l'Amstrad  PCW  utilise");
    locate(0, 7);
    print("la norme ASCII  {tendue");
    locate(0, 8);
    print("@ 256  caract}res  mais");
    locate(0, 9);
    print("certains sont remplac{s");
    locate(0, 10);
    print("par    des   caract}res");
    locate(0, 11);
    print("accentu{s.");

    locate(0, 13);
    print("Le  jeu  de  caract}res");
    locate(0, 14);
    print("est  orient{ traitement");
    locate(0, 15);
    print("de texte.");

    frame(0, 5*8, 23*8, 16*8);

    // Print every character (except character 0)
    set_size(SIZE_DOUBLE);
    for(j = 0; j < 16; j++) {
        for(i = 0; i < 16; i++) {
            line[i * 2] = (j << 4) + i;
        }

        if(line[0] == '\0') line[0] = ' ';

        locate(90-65, j * 2);
        print(line);
    }

    for(i = 0; i < 16; i++) {
        vertical_line(i * 32 + 24 * 8, 0, 255);
        horizontal_line(24 * 8, 703, i * 16);
    }

    vertical_line(704, 0, 255);
    horizontal_line(24 * 8, 703, 255);

    /*
    // Draw vertical lines
    for(x = 10; x < 710; x++) {
        vertical_line(x, 10, 245);
    }

    // Draw horizontal lines
    for(y = 20; y < 100; y++) {
        horizontal_line(500-y, 500+y, y);
    }
    */

    /*
    for(x = 10; x < 120; x+= 10) {
        frame(x * 2, x, 720 - x * 2, 255 - x);
    }
    */

    // Wait for a key before returning to CP/M
    getchar();

    // Restore standard screen settings
    restore_video_ram();

    return 0;
}
