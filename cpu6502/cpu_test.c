#include <stdio.h>
#include <string.h>

#include "fake6502.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "minimal_computer.h"
#include "pico/multicore.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/stdlib.h"
#include "vga/vga.h"

// array size is 2048
static const uint8_t chardefs[] = {
    0x5a, 0x99, 0xe7, 0x5e, 0x5e, 0x24, 0x18, 0x66, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,
    0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0,
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x55, 0xaa, 0x55,
    0xaa, 0x55, 0xaa, 0x55, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x18, 0x3c, 0x7e, 0xff, 0x7e, 0x66, 0x66, 0x66,
    0x00, 0x08, 0x0c, 0xff, 0xff, 0x0c, 0x08, 0x00, 0x00, 0x10, 0x30, 0xff, 0xff, 0x30, 0x10, 0x00,
    0x18, 0x18, 0x3c, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x18,
    0x7e, 0x99, 0x99, 0xff, 0xff, 0x99, 0x99, 0x7e, 0x18, 0x24, 0x42, 0x99, 0x99, 0x42, 0x24, 0x18,
    0x00, 0x24, 0x24, 0x00, 0x81, 0x42, 0x3c, 0x00, 0x00, 0x24, 0x24, 0x00, 0x3c, 0x42, 0x81, 0x00,
    0x3c, 0x7e, 0x99, 0xff, 0xe7, 0x7e, 0x3c, 0x66, 0x3c, 0x7e, 0x99, 0xdd, 0xff, 0xff, 0xff, 0xdb,
    0x3c, 0x42, 0x84, 0x88, 0x88, 0x84, 0x42, 0x3c, 0x42, 0x24, 0x7e, 0xdb, 0xff, 0xbd, 0xa5, 0x18,
    0x18, 0x3c, 0x3c, 0x7e, 0x7e, 0xff, 0x99, 0x18, 0x00, 0x80, 0xe0, 0xbc, 0xff, 0x78, 0x60, 0x00,
    0x00, 0x00, 0x24, 0x18, 0x18, 0x24, 0x00, 0x00, 0x81, 0x42, 0x24, 0x00, 0x00, 0x24, 0x42, 0x81,
    0x00, 0x00, 0x3c, 0xc3, 0xc3, 0x3c, 0x00, 0x00, 0x18, 0x18, 0x24, 0x24, 0x24, 0x24, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00,
    0x00, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x7e, 0x24, 0x24, 0x7e, 0x24, 0x00,
    0x00, 0x08, 0x3e, 0x28, 0x3e, 0x0a, 0x3e, 0x08, 0x00, 0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00,
    0x00, 0x10, 0x28, 0x10, 0x2a, 0x44, 0x3a, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x08, 0x08, 0x08, 0x08, 0x04, 0x00, 0x00, 0x20, 0x10, 0x10, 0x10, 0x10, 0x20, 0x00,
    0x00, 0x00, 0x14, 0x08, 0x3e, 0x08, 0x14, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00,
    0x00, 0x3c, 0x46, 0x4a, 0x52, 0x62, 0x3c, 0x00, 0x00, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3e, 0x00,
    0x00, 0x3c, 0x42, 0x02, 0x3c, 0x40, 0x7e, 0x00, 0x00, 0x3c, 0x42, 0x0c, 0x02, 0x42, 0x3c, 0x00,
    0x00, 0x08, 0x18, 0x28, 0x48, 0x7e, 0x08, 0x00, 0x00, 0x7e, 0x40, 0x7c, 0x02, 0x42, 0x3c, 0x00,
    0x00, 0x3c, 0x40, 0x7c, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00,
    0x00, 0x3c, 0x42, 0x3c, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x3e, 0x02, 0x3c, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20,
    0x00, 0x00, 0x04, 0x08, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x00, 0x3c, 0x42, 0x04, 0x08, 0x00, 0x08, 0x00,
    0x00, 0x3c, 0x4a, 0x56, 0x5e, 0x40, 0x3c, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00,
    0x00, 0x7c, 0x42, 0x7c, 0x42, 0x42, 0x7c, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 0x42, 0x3c, 0x00,
    0x00, 0x78, 0x44, 0x42, 0x42, 0x44, 0x78, 0x00, 0x00, 0x7e, 0x40, 0x7c, 0x40, 0x40, 0x7e, 0x00,
    0x00, 0x7e, 0x40, 0x7c, 0x40, 0x40, 0x40, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x4e, 0x42, 0x3c, 0x00,
    0x00, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00, 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x3e, 0x00,
    0x00, 0x02, 0x02, 0x02, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00,
    0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x42, 0x66, 0x5a, 0x42, 0x42, 0x42, 0x00,
    0x00, 0x42, 0x62, 0x52, 0x4a, 0x46, 0x42, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00,
    0x00, 0x7c, 0x42, 0x42, 0x7c, 0x40, 0x40, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x52, 0x4a, 0x3c, 0x00,
    0x00, 0x7c, 0x42, 0x42, 0x7c, 0x44, 0x42, 0x00, 0x00, 0x3c, 0x40, 0x3c, 0x02, 0x42, 0x3c, 0x00,
    0x00, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00,
    0x00, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x5a, 0x24, 0x00,
    0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 0x00, 0x82, 0x44, 0x28, 0x10, 0x10, 0x10, 0x00,
    0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 0x7e, 0x00, 0x00, 0x0e, 0x08, 0x08, 0x08, 0x08, 0x0e, 0x00,
    0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00, 0x70, 0x10, 0x10, 0x10, 0x10, 0x70, 0x00,
    0x00, 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3c, 0x00,
    0x00, 0x20, 0x20, 0x3c, 0x22, 0x22, 0x3c, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x20, 0x20, 0x1c, 0x00,
    0x00, 0x04, 0x04, 0x3c, 0x44, 0x44, 0x3c, 0x00, 0x00, 0x00, 0x38, 0x44, 0x78, 0x40, 0x3c, 0x00,
    0x00, 0x0c, 0x10, 0x18, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x3c, 0x44, 0x44, 0x3c, 0x04, 0x38,
    0x00, 0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x00, 0x00, 0x10, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00,
    0x00, 0x04, 0x00, 0x04, 0x04, 0x04, 0x24, 0x18, 0x00, 0x20, 0x28, 0x30, 0x30, 0x28, 0x24, 0x00,
    0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0c, 0x00, 0x00, 0x00, 0x68, 0x54, 0x54, 0x54, 0x54, 0x00,
    0x00, 0x00, 0x78, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00,
    0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x00, 0x00, 0x3c, 0x44, 0x44, 0x3c, 0x04, 0x06,
    0x00, 0x00, 0x1c, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x38, 0x40, 0x38, 0x04, 0x78, 0x00,
    0x00, 0x10, 0x38, 0x10, 0x10, 0x10, 0x0c, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00,
    0x00, 0x00, 0x44, 0x44, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00, 0x44, 0x54, 0x54, 0x54, 0x28, 0x00,
    0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x3c, 0x04, 0x38,
    0x00, 0x00, 0x7c, 0x08, 0x10, 0x20, 0x7c, 0x00, 0x00, 0x0e, 0x08, 0x30, 0x08, 0x08, 0x0e, 0x00,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x70, 0x10, 0x0c, 0x10, 0x10, 0x70, 0x00,
    0x00, 0x14, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x99, 0xa1, 0xa1, 0x99, 0x42, 0x3c,
    0xa5, 0x66, 0x18, 0xa1, 0xa1, 0xdb, 0xe7, 0x99, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff,
    0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f,
    0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0xff, 0xff, 0xff, 0xff, 0x55, 0xaa, 0x55, 0xaa,
    0x55, 0xaa, 0x55, 0xaa, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,
    0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe7, 0xc3, 0x81, 0x00, 0x81, 0x99, 0x99, 0x99,
    0xff, 0xf7, 0xf3, 0x00, 0x00, 0xf3, 0xf7, 0xff, 0xff, 0xef, 0xcf, 0x00, 0x00, 0xcf, 0xef, 0xff,
    0xe7, 0xe7, 0xc3, 0x81, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x81, 0xc3, 0xe7, 0xe7,
    0x81, 0x66, 0x66, 0x00, 0x00, 0x66, 0x66, 0x81, 0xe7, 0xdb, 0xbd, 0x66, 0x66, 0xbd, 0xdb, 0xe7,
    0xff, 0xdb, 0xdb, 0xff, 0x7e, 0xbd, 0xc3, 0xff, 0xff, 0xdb, 0xdb, 0xff, 0xc3, 0xbd, 0x7e, 0xff,
    0xc3, 0x81, 0x66, 0x00, 0x18, 0x81, 0xc3, 0x99, 0xc3, 0x81, 0x66, 0x22, 0x00, 0x00, 0x00, 0x24,
    0xc3, 0xbd, 0x7b, 0x77, 0x77, 0x7b, 0xbd, 0xc3, 0xbd, 0xdb, 0x81, 0x24, 0x00, 0x42, 0x5a, 0xe7,
    0xe7, 0xc3, 0xc3, 0x81, 0x81, 0x00, 0x66, 0xe7, 0xff, 0x7f, 0x1f, 0x43, 0x00, 0x87, 0x9f, 0xff,
    0xff, 0xff, 0xdb, 0xe7, 0xe7, 0xdb, 0xff, 0xff, 0x7e, 0xbd, 0xdb, 0xff, 0xff, 0xdb, 0xbd, 0x7e,
    0xff, 0xff, 0xc3, 0x3c, 0x3c, 0xc3, 0xff, 0xff, 0xe7, 0xe7, 0xdb, 0xdb, 0xdb, 0xdb, 0xe7, 0xe7,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xef, 0xef, 0xef, 0xff, 0xef, 0xff,
    0xff, 0xdb, 0xdb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdb, 0x81, 0xdb, 0xdb, 0x81, 0xdb, 0xff,
    0xff, 0xf7, 0xc1, 0xd7, 0xc1, 0xf5, 0xc1, 0xf7, 0xff, 0x9d, 0x9b, 0xf7, 0xef, 0xd9, 0xb9, 0xff,
    0xff, 0xef, 0xd7, 0xef, 0xd5, 0xbb, 0xc5, 0xff, 0xff, 0xf7, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xfb, 0xf7, 0xf7, 0xf7, 0xf7, 0xfb, 0xff, 0xff, 0xdf, 0xef, 0xef, 0xef, 0xef, 0xdf, 0xff,
    0xff, 0xff, 0xeb, 0xf7, 0xc1, 0xf7, 0xeb, 0xff, 0xff, 0xff, 0xf7, 0xf7, 0xc1, 0xf7, 0xf7, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xf7, 0xef, 0xff, 0xff, 0xff, 0xff, 0xc1, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xff,
    0xff, 0xc3, 0xb9, 0xb5, 0xad, 0x9d, 0xc3, 0xff, 0xff, 0xe7, 0xd7, 0xf7, 0xf7, 0xf7, 0xc1, 0xff,
    0xff, 0xc3, 0xbd, 0xfd, 0xc3, 0xbf, 0x81, 0xff, 0xff, 0xc3, 0xbd, 0xf3, 0xfd, 0xbd, 0xc3, 0xff,
    0xff, 0xf7, 0xe7, 0xd7, 0xb7, 0x81, 0xf7, 0xff, 0xff, 0x81, 0xbf, 0x83, 0xfd, 0xbd, 0xc3, 0xff,
    0xff, 0xc3, 0xbf, 0x83, 0xbd, 0xbd, 0xc3, 0xff, 0xff, 0x81, 0xfd, 0xfb, 0xf7, 0xef, 0xef, 0xff,
    0xff, 0xc3, 0xbd, 0xc3, 0xbd, 0xbd, 0xc3, 0xff, 0xff, 0xc3, 0xbd, 0xbd, 0xc1, 0xfd, 0xc3, 0xff,
    0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xef, 0xef, 0xdf,
    0xff, 0xff, 0xfb, 0xf7, 0xef, 0xf7, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xc1, 0xff, 0xc1, 0xff, 0xff,
    0xff, 0xff, 0xef, 0xf7, 0xfb, 0xf7, 0xef, 0xff, 0xff, 0xc3, 0xbd, 0xfb, 0xf7, 0xff, 0xf7, 0xff,
    0xff, 0xc3, 0xb5, 0xa9, 0xa1, 0xbf, 0xc3, 0xff, 0xff, 0xc3, 0xbd, 0xbd, 0x81, 0xbd, 0xbd, 0xff,
    0xff, 0x83, 0xbd, 0x83, 0xbd, 0xbd, 0x83, 0xff, 0xff, 0xc3, 0xbd, 0xbf, 0xbf, 0xbd, 0xc3, 0xff,
    0xff, 0x87, 0xbb, 0xbd, 0xbd, 0xbb, 0x87, 0xff, 0xff, 0x81, 0xbf, 0x83, 0xbf, 0xbf, 0x81, 0xff,
    0xff, 0x81, 0xbf, 0x83, 0xbf, 0xbf, 0xbf, 0xff, 0xff, 0xc3, 0xbd, 0xbf, 0xb1, 0xbd, 0xc3, 0xff,
    0xff, 0xbd, 0xbd, 0x81, 0xbd, 0xbd, 0xbd, 0xff, 0xff, 0xc1, 0xf7, 0xf7, 0xf7, 0xf7, 0xc1, 0xff,
    0xff, 0xfd, 0xfd, 0xfd, 0xbd, 0xbd, 0xc3, 0xff, 0xff, 0xbb, 0xb7, 0x8f, 0xb7, 0xbb, 0xbd, 0xff,
    0xff, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0x81, 0xff, 0xff, 0xbd, 0x99, 0xa5, 0xbd, 0xbd, 0xbd, 0xff,
    0xff, 0xbd, 0x9d, 0xad, 0xb5, 0xb9, 0xbd, 0xff, 0xff, 0xc3, 0xbd, 0xbd, 0xbd, 0xbd, 0xc3, 0xff,
    0xff, 0x83, 0xbd, 0xbd, 0x83, 0xbf, 0xbf, 0xff, 0xff, 0xc3, 0xbd, 0xbd, 0xad, 0xb5, 0xc3, 0xff,
    0xff, 0x83, 0xbd, 0xbd, 0x83, 0xbb, 0xbd, 0xff, 0xff, 0xc3, 0xbf, 0xc3, 0xfd, 0xbd, 0xc3, 0xff,
    0xff, 0x01, 0xef, 0xef, 0xef, 0xef, 0xef, 0xff, 0xff, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xc3, 0xff,
    0xff, 0xbd, 0xbd, 0xbd, 0xbd, 0xdb, 0xe7, 0xff, 0xff, 0xbd, 0xbd, 0xbd, 0xbd, 0xa5, 0xdb, 0xff,
    0xff, 0xbd, 0xdb, 0xe7, 0xe7, 0xdb, 0xbd, 0xff, 0xff, 0x7d, 0xbb, 0xd7, 0xef, 0xef, 0xef, 0xff,
    0xff, 0x81, 0xfb, 0xf7, 0xef, 0xdf, 0x81, 0xff, 0xff, 0xf1, 0xf7, 0xf7, 0xf7, 0xf7, 0xf1, 0xff,
    0xff, 0xff, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xff, 0xff, 0x8f, 0xef, 0xef, 0xef, 0xef, 0x8f, 0xff,
    0xff, 0xef, 0xc7, 0xab, 0xef, 0xef, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0xff, 0xef, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xfb, 0xc3, 0xbb, 0xc3, 0xff,
    0xff, 0xdf, 0xdf, 0xc3, 0xdd, 0xdd, 0xc3, 0xff, 0xff, 0xff, 0xe3, 0xdf, 0xdf, 0xdf, 0xe3, 0xff,
    0xff, 0xfb, 0xfb, 0xc3, 0xbb, 0xbb, 0xc3, 0xff, 0xff, 0xff, 0xc7, 0xbb, 0x87, 0xbf, 0xc3, 0xff,
    0xff, 0xf3, 0xef, 0xe7, 0xef, 0xef, 0xef, 0xff, 0xff, 0xff, 0xc3, 0xbb, 0xbb, 0xc3, 0xfb, 0xc7,
    0xff, 0xbf, 0xbf, 0x87, 0xbb, 0xbb, 0xbb, 0xff, 0xff, 0xef, 0xff, 0xcf, 0xef, 0xef, 0xc7, 0xff,
    0xff, 0xfb, 0xff, 0xfb, 0xfb, 0xfb, 0xdb, 0xe7, 0xff, 0xdf, 0xd7, 0xcf, 0xcf, 0xd7, 0xdb, 0xff,
    0xff, 0xef, 0xef, 0xef, 0xef, 0xef, 0xf3, 0xff, 0xff, 0xff, 0x97, 0xab, 0xab, 0xab, 0xab, 0xff,
    0xff, 0xff, 0x87, 0xbb, 0xbb, 0xbb, 0xbb, 0xff, 0xff, 0xff, 0xc7, 0xbb, 0xbb, 0xbb, 0xc7, 0xff,
    0xff, 0xff, 0x87, 0xbb, 0xbb, 0x87, 0xbf, 0xbf, 0xff, 0xff, 0xc3, 0xbb, 0xbb, 0xc3, 0xfb, 0xf9,
    0xff, 0xff, 0xe3, 0xdf, 0xdf, 0xdf, 0xdf, 0xff, 0xff, 0xff, 0xc7, 0xbf, 0xc7, 0xfb, 0x87, 0xff,
    0xff, 0xef, 0xc7, 0xef, 0xef, 0xef, 0xf3, 0xff, 0xff, 0xff, 0xbb, 0xbb, 0xbb, 0xbb, 0xc7, 0xff,
    0xff, 0xff, 0xbb, 0xbb, 0xd7, 0xd7, 0xef, 0xff, 0xff, 0xff, 0xbb, 0xab, 0xab, 0xab, 0xd7, 0xff,
    0xff, 0xff, 0xbb, 0xd7, 0xef, 0xd7, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xbb, 0xbb, 0xc3, 0xfb, 0xc7,
    0xff, 0xff, 0x83, 0xf7, 0xef, 0xdf, 0x83, 0xff, 0xff, 0xf1, 0xf7, 0xcf, 0xf7, 0xf7, 0xf1, 0xff,
    0xff, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xff, 0xff, 0x8f, 0xef, 0xf3, 0xef, 0xef, 0x8f, 0xff,
    0xff, 0xeb, 0xd7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0xbd, 0x66, 0x5e, 0x5e, 0x66, 0xbd, 0xc3};

int main() {
    set_sys_clock_201_6mhz();
    stdio_init_all();

    measure_freqs();

    memcpy(mode0_charmem, chardefs, 2048);

    for (int i = 0; i < 1200; i++) {
        mode0_vidmem[i] = i % 256;
    }

    // launch all the video on core 1
    multicore_launch_core1(mode0_loop);

    // wait for initialization of video to be complete
    sem_acquire_blocking(&video_initted);

    while (true) {
    }

    // getchar_timeout_us(0);

    // Load a ROM into the mem array...
    // from https://skilldrick.github.io/easy6502/
    // 0600: a9 01 8d 00 02 a9 05 8d 01 02 a9 08 8d 02 02
    // static const uint8_t program[] = { 0xa9, 0x01, 0x8d, 0x00, 0x02, 0xa9,
    // 0x05, 0x8d, 0x01, 0x02, 0xa9, 0x08, 0x8d, 0x02, 0x02 };

    static const uint8_t program[] = {
        0x20, 0x06, 0x06, 0x20, 0x38, 0x06, 0x20, 0x0d, 0x06, 0x20, 0x2a, 0x06, 0x60, 0xa9, 0x02,
        0x85, 0x02, 0xa9, 0x04, 0x85, 0x03, 0xa9, 0x11, 0x85, 0x10, 0xa9, 0x10, 0x85, 0x12, 0xa9,
        0x0f, 0x85, 0x14, 0xa9, 0x04, 0x85, 0x11, 0x85, 0x13, 0x85, 0x15, 0x60, 0xa5, 0xfe, 0x85,
        0x00, 0xa5, 0xfe, 0x29, 0x03, 0x18, 0x69, 0x02, 0x85, 0x01, 0x60, 0x20, 0x4d, 0x06, 0x20,
        0x8d, 0x06, 0x20, 0xc3, 0x06, 0x20, 0x19, 0x07, 0x20, 0x20, 0x07, 0x20, 0x2d, 0x07, 0x4c,
        0x38, 0x06, 0xa5, 0xff, 0xc9, 0x77, 0xf0, 0x0d, 0xc9, 0x64, 0xf0, 0x14, 0xc9, 0x73, 0xf0,
        0x1b, 0xc9, 0x61, 0xf0, 0x22, 0x60, 0xa9, 0x04, 0x24, 0x02, 0xd0, 0x26, 0xa9, 0x01, 0x85,
        0x02, 0x60, 0xa9, 0x08, 0x24, 0x02, 0xd0, 0x1b, 0xa9, 0x02, 0x85, 0x02, 0x60, 0xa9, 0x01,
        0x24, 0x02, 0xd0, 0x10, 0xa9, 0x04, 0x85, 0x02, 0x60, 0xa9, 0x02, 0x24, 0x02, 0xd0, 0x05,
        0xa9, 0x08, 0x85, 0x02, 0x60, 0x60, 0x20, 0x94, 0x06, 0x20, 0xa8, 0x06, 0x60, 0xa5, 0x00,
        0xc5, 0x10, 0xd0, 0x0d, 0xa5, 0x01, 0xc5, 0x11, 0xd0, 0x07, 0xe6, 0x03, 0xe6, 0x03, 0x20,
        0x2a, 0x06, 0x60, 0xa2, 0x02, 0xb5, 0x10, 0xc5, 0x10, 0xd0, 0x06, 0xb5, 0x11, 0xc5, 0x11,
        0xf0, 0x09, 0xe8, 0xe8, 0xe4, 0x03, 0xf0, 0x06, 0x4c, 0xaa, 0x06, 0x4c, 0x35, 0x07, 0x60,
        0xa6, 0x03, 0xca, 0x8a, 0xb5, 0x10, 0x95, 0x12, 0xca, 0x10, 0xf9, 0xa5, 0x02, 0x4a, 0xb0,
        0x09, 0x4a, 0xb0, 0x19, 0x4a, 0xb0, 0x1f, 0x4a, 0xb0, 0x2f, 0xa5, 0x10, 0x38, 0xe9, 0x20,
        0x85, 0x10, 0x90, 0x01, 0x60, 0xc6, 0x11, 0xa9, 0x01, 0xc5, 0x11, 0xf0, 0x28, 0x60, 0xe6,
        0x10, 0xa9, 0x1f, 0x24, 0x10, 0xf0, 0x1f, 0x60, 0xa5, 0x10, 0x18, 0x69, 0x20, 0x85, 0x10,
        0xb0, 0x01, 0x60, 0xe6, 0x11, 0xa9, 0x06, 0xc5, 0x11, 0xf0, 0x0c, 0x60, 0xc6, 0x10, 0xa5,
        0x10, 0x29, 0x1f, 0xc9, 0x1f, 0xf0, 0x01, 0x60, 0x4c, 0x35, 0x07, 0xa0, 0x00, 0xa5, 0xfe,
        0x91, 0x00, 0x60, 0xa6, 0x03, 0xa9, 0x00, 0x81, 0x10, 0xa2, 0x00, 0xa9, 0x07, 0x81, 0x10,
        0x60, 0xa2, 0x00, 0xea, 0xea, 0xca, 0xd0, 0xfb, 0x60};

    memcpy(mem + 0x0600, program, sizeof(program));

    // 0600: 20 06 06 20 38 06 20 0d 06 20 2a 06 60 a9 02 85
    // 0610: 02 a9 04 85 03 a9 11 85 10 a9 10 85 12 a9 0f 85
    // 0620: 14 a9 04 85 11 85 13 85 15 60 a5 fe 85 00 a5 fe
    // 0630: 29 03 18 69 02 85 01 60 20 4d 06 20 8d 06 20 c3
    // 0640: 06 20 19 07 20 20 07 20 2d 07 4c 38 06 a5 ff c9
    // 0650: 77 f0 0d c9 64 f0 14 c9 73 f0 1b c9 61 f0 22 60
    // 0660: a9 04 24 02 d0 26 a9 01 85 02 60 a9 08 24 02 d0
    // 0670: 1b a9 02 85 02 60 a9 01 24 02 d0 10 a9 04 85 02
    // 0680: 60 a9 02 24 02 d0 05 a9 08 85 02 60 60 20 94 06
    // 0690: 20 a8 06 60 a5 00 c5 10 d0 0d a5 01 c5 11 d0 07
    // 06a0: e6 03 e6 03 20 2a 06 60 a2 02 b5 10 c5 10 d0 06
    // 06b0: b5 11 c5 11 f0 09 e8 e8 e4 03 f0 06 4c aa 06 4c
    // 06c0: 35 07 60 a6 03 ca 8a b5 10 95 12 ca 10 f9 a5 02
    // 06d0: 4a b0 09 4a b0 19 4a b0 1f 4a b0 2f a5 10 38 e9
    // 06e0: 20 85 10 90 01 60 c6 11 a9 01 c5 11 f0 28 60 e6
    // 06f0: 10 a9 1f 24 10 f0 1f 60 a5 10 18 69 20 85 10 b0
    // 0700: 01 60 e6 11 a9 06 c5 11 f0 0c 60 c6 10 a5 10 29
    // 0710: 1f c9 1f f0 01 60 4c 35 07 a0 00 a5 fe 91 00 60
    // 0720: a6 03 a9 00 81 10 a2 00 a9 01 81 10 60 a2 00 ea
    // 0730: ea ca d0 fb 60

    mem[0xfffc] = 0x00;
    mem[0xfffd] = 0x06;

    // NOTE more programs at https://github.com/r00ster91/easy6502

#if 0
        unsigned char str[40];
        size_t readLine() {
            unsigned char u, *p;
            for (p = str, u = getchar(); u != '\r' && p - str < 39; u = getchar()) {
                putchar(*p++ = u);
            }
            *p++ = 0;
            return p - str;
        }

        // there seems to always be one character in the input? so I guess we'll clear it...
        getchar_timeout_us(100);

        while (1) {
            printf("> ");
            stdio_flush();
            size_t size = readLine();
            printf("\nRead %d bytes: %s\n", size, str);
        }

        /*
         char action[4];
         int addr;
         int n=1;

         while (1) {
             int count = 0;
             int a[100];
        // Perform a do-while loop
         do {

             // Take input at position count
             // and increment count
             scanf("%d", &a[count++]);
             printf("C: %d %d\n", count, getchar());


             // If '\n' (newline) has occurred
             // or the whole array is filled,
             // then exit the loop

             // Otherwise, continue
         } while (getchar() != '\n' && count < 100);
             printf("STOP\n");

             int result = scanf("%s %x %d", action, &addr, &n);

             //printf("%d\n", result);
             printf("%04x: ", addr);
             for (int i=0; i<n; i++) {
                 printf("%02x ", mem[addr++]);
             }
             printf("\n");

             if (!strcmp(action, "r")) {
                 // read6502(addr);
             }
         }
         */
#endif

    reset6502();

    while (1) {
        exec6502(10);
        int ch = getchar_timeout_us(1000);
        if (ch != PICO_ERROR_TIMEOUT) {
            mem[0xff] = ch;
        }
    }
}