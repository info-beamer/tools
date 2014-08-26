// Copyright (c) 2014 Florian Wesch <fw@dividuum.de> 
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

#include "bcm_host.h"
#include "jpeglib.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

int main(int argc, char *argv[]) {
    bcm_host_init();

    uint32_t screen = 0;

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(screen);
    DISPMANX_MODEINFO_T info;

    int ret = vc_dispmanx_display_get_info(display, &info);
    assert(ret == 0);

    /* DispmanX expects buffer rows to be aligned to a 32 bit boundary */
    int pitch = ALIGN_UP(2 * info.width, 32);

    uint32_t vc_image_ptr;
    VC_IMAGE_TYPE_T type = VC_IMAGE_RGB565;
    DISPMANX_RESOURCE_HANDLE_T resource = vc_dispmanx_resource_create(
        type, info.width, info.height, &vc_image_ptr
    );
    
    VC_IMAGE_TRANSFORM_T transform = 0;
    vc_dispmanx_snapshot(display, resource, transform);

    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);

    unsigned char *image = malloc(pitch * info.height);
    assert(image);
    vc_dispmanx_resource_read_data(resource, &rect, image, info.width * 2);

    ret = vc_dispmanx_resource_delete(resource);
    assert(ret == 0);

    ret = vc_dispmanx_display_close(display);
    assert(ret == 0);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, stdout);

    cinfo.image_width = info.width;
    cinfo.image_height = info.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 20, TRUE);
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
        row_pointer[0] = row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return 0;
}
