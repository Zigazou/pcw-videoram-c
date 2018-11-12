#include <stdio.h>
#include <stdlib.h>

#pragma output noprotectmsdos
#pragma output noredir
#pragma output nogfxglobals

struct {
    // Number of 128-byte records per track
    unsigned int records_per_track;

    // Block shift. 3 => 1k, 4 => 2k, 5 => 4k....
    unsigned char block_shift;

    // Block mask. 7 => 1k, 0Fh => 2k, 1Fh => 4k...
    unsigned char block_mask;

	// Extent mask, see later
    unsigned char extent_mask;

    // no. of blocks on the disc - 1
    unsigned int nb_disk_blocks;

    // no. of directory entries - 1
    unsigned int nb_directory_entries;

    // Directory allocation bitmap
    unsigned int dir_allocation_bitmap;

    // Checksum vector size, 0 or 8000h for a fixed disc.
    // No. directory entries/4, rounded up.
    unsigned int checksum_vector_size;

    // Offset, number of reserved tracks
    unsigned int reserved_tracks;

    // Physical sector shift, 0 => 128-byte sectors
    // 1 => 256-byte sectors  2 => 512-byte sectors...
    unsigned char physical_sector_shift;

    // Physical sector mask,  0 => 128-byte sectors
    // 1 => 256-byte sectors, 3 => 512-byte sectors...
    unsigned char physical_sector_mask;
} *dpb;

char *block_shift[] = {
    "128", "256", "512", "1024", "2048", "4096", "8192", "16384",
    "32768", "65536", "131072", "262144", "524288"
};

char *block_mask(unsigned char value) {
    switch(value) {
        case 0x01: return "256";
        case 0x03: return "512";
        case 0x07: return "1024";
        case 0x0f: return "2048";
        case 0x1f: return "4096";
        case 0x3f: return "8192";
        case 0x7f: return "16384";
        case 0xff: return "32768";
        default: return "128";
    }
}

 main() {
    dpb = (void *)bdos(31, 0);

    printf(
        "Records per track:                 %d\n",
        dpb->records_per_track
    );

    printf(
        "Block shift:                       %s bytes (%d)\n",
        block_shift[dpb->block_shift],
        dpb->block_shift
    );

    printf(
        "Block mask:                        %s bytes (%d)\n",
        block_mask(dpb->block_mask),
        dpb->block_mask
    );

    printf(
        "Extent mask:                       (%d)\n",
        dpb->extent_mask
    );

    if(dpb->nb_disk_blocks != 65535) {
        printf(
            "Number of blocks on the disc:      %d\n",
            dpb->nb_disk_blocks + 1
        );
    } else {
        printf(
            "Number of blocks on the disc:      65536\n"
        );
    }

    if(dpb->nb_directory_entries != 65535) {
        printf(
            "Number of directory entries:       %d\n",
            dpb->nb_directory_entries + 1
        );
    } else {
        printf(
            "Number of directory entries:       65536\n"
        );
    }

    printf(
        "Directory allocation bitmap:       (%d)\n",
        dpb->dir_allocation_bitmap
    );

    printf(
        "Offset, number of reserved tracks: %d\n",
        dpb->reserved_tracks
    );

    printf(
        "Physical sector shift:             %d-byte sectors (%d)\n",
        block_shift[dpb->physical_sector_shift],
        dpb->physical_sector_shift
    );

    printf(
        "Physical sector mask:              %d-byte sectors (%d)\n",
        block_shift[dpb->physical_sector_mask],
        dpb->physical_sector_mask
    );

    return 0;
}
