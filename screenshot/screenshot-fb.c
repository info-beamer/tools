// Copyright (c) 2024 Florian Wesch <fw@dividuum.de> 
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer. 
//
//     Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the
//     distribution.  
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jpeglib.h"

int main(int argc, char *argv[]) {
    int quality = 75;
    if (argc >= 2) {
        quality = atoi(argv[1]);
        if (quality < 10 || quality > 100) {
            fprintf(stderr, "invalid quality. must be between 10 and 100\n");
            return 1;
        }
    }

    int fbfd = open("/dev/fb0", O_RDWR);
    assert(fbfd > 0);

    struct fb_var_screeninfo info;
    ioctl(fbfd, FBIOGET_VSCREENINFO, &info);
    assert(info.bits_per_pixel == 16);

    size_t map_size = info.xres * info.yres * info.bits_per_pixel / 8;
    int pitch = info.xres * info.bits_per_pixel / 8;
    unsigned char *image = mmap(NULL, map_size, PROT_READ, MAP_SHARED, fbfd, 0);
    assert(image != MAP_FAILED);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, stdout);

    cinfo.image_width = info.xres;
    cinfo.image_height = info.yres;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = cinfo.image_width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned char row[row_stride];
        unsigned char *dst = &row[0];
        uint16_t *src = (uint16_t*)(image + pitch * cinfo.next_scanline);
        for (int x = 0; x < cinfo.image_width; x++, src++) {
            *dst++ = ((*src & 0xf800) >> 11) << 3;
            *dst++ = ((*src & 0x07e0) >>  5) << 2;
            *dst++ = ((*src & 0x001f) >>  0) << 3;
        }
        JSAMPROW row_pointer[1] = {row};
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return 0;
}
