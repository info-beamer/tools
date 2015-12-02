// Copyright (c) 2015 Florian Wesch <fw@dividuum.de> 
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
#include <stdarg.h>

#include "bcm_host.h"
#include "jpeglib.h"

#define ALIGN_TO_16(x)  ((x + 15) & ~15)

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("error: ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    exit(1); 
}

static DISPMANX_DISPLAY_HANDLE_T display;
static DISPMANX_RESOURCE_HANDLE_T image;
static DISPMANX_ELEMENT_HANDLE_T element;

static void show_image(const char *filename, int screen, int layer) {
    display = vc_dispmanx_display_open(screen);

    DISPMANX_MODEINFO_T info;
    if (vc_dispmanx_display_get_info(display, &info) != 0)
        die("cannot get display info");

    printf("screen size: %d %d\n", info.width, info.height);

    FILE *in = fopen(filename, "rb");
    if (!in)
        die("cannot open input file: %s", strerror(errno));

    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, in);
    jpeg_read_header(&cinfo, TRUE);

    const int w = ALIGN_TO_16(cinfo.image_width);
    const int h = ALIGN_TO_16(cinfo.image_height);
    const int bpp = cinfo.num_components;
    const int stride = w * bpp;

    printf("jpeg size: %d %d\n", w, h);

    if (cinfo.jpeg_color_space != JCS_RGB &&
        cinfo.jpeg_color_space != JCS_YCbCr)
        die("invalid jpeg color space");

    jpeg_start_decompress(&cinfo);
    unsigned char *pixels  = malloc(w*h*bpp);
    if (!pixels)
        die("cannot allocate pixel memory");

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char * rowptr[] = {pixels + cinfo.output_scanline * stride};
        jpeg_read_scanlines(&cinfo, rowptr, 1);
    }
    jpeg_destroy_decompress(&cinfo);

    VC_IMAGE_TYPE_T image_type = VC_IMAGE_RGB888;

    uint32_t image_ptr;
    image = vc_dispmanx_resource_create(image_type, w | stride<<16, h | h<<16, &image_ptr);
    if (image == 0)
        die("cannot setup image layer");

    VC_RECT_T dst_rect, src_rect;
    vc_dispmanx_rect_set(&dst_rect, 0, 0, w, h);
    vc_dispmanx_resource_write_data(image, image_type, stride, pixels, &dst_rect);
    free(pixels);

    vc_dispmanx_rect_set(&src_rect, 0, 0, cinfo.image_width<<16, cinfo.image_height<<16);
    vc_dispmanx_rect_set(&dst_rect, 0, 0, 0, 0);

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(1);
    element = vc_dispmanx_element_add(
        update,
        display,
        layer,
        &dst_rect,
        image,
        &src_rect,
        DISPMANX_PROTECTION_NONE,
        0 /*alpha*/,
        0 /*clamp*/,
        DISPMANX_NO_ROTATE
    );
    if (!element)
        die("cannot get element");

    int ret = vc_dispmanx_update_submit_sync(update);
    if (ret != 0)
        die("cannot create layer");
}

int main(int argc, char *argv[]) {
    bcm_host_init();

    int screen = 0;
    if (getenv("SCREEN"))
        screen = atoi(getenv("SCREEN"));

    int layer = -1;
    if (getenv("LAYER"))
        layer = atoi(getenv("LAYER"));

    if (argc != 2)
        die("need jpg file argument");

    show_image(argv[1], screen, layer);

    while (1)
        sleep(10);
    return 0;
}
