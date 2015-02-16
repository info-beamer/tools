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
#include <signal.h>
#include <unistd.h>

#include "bcm_host.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n"); 
    va_end(ap);
    exit(1);
}

static int env_num(const char *name, int min, int max) {
    const char *val = getenv(name);
    if (!val)
        die("missing value for %s", name);
    char *endptr;
    int intval = strtol(val, &endptr, 10);
    if (endptr == val)
        die("invalid number");
    if (intval < min || intval > max)
        die("invalud value %d. must be in range [%d, %d]", intval, min, max);
    return intval;
}

static int running = 1;

static void shutdown_signal(int i) {
    running = 0;
}

int main(int argc, char *argv[]) {
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);

    sig.sa_handler = shutdown_signal;
    sig.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sig, NULL);
    sigaction(SIGTERM, &sig, NULL);

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
    DISPMANX_MODEINFO_T info;

    int ret = vc_dispmanx_display_get_info(display, &info);
    if (ret != 0)
        die("cannot get_info");

    const int pitch = ALIGN_UP(info.width * 3, 32);

    uint32_t vc_image_ptr;
    VC_IMAGE_TYPE_T type = VC_IMAGE_RGB888;
    DISPMANX_RESOURCE_HANDLE_T resource = vc_dispmanx_resource_create(
        type, info.width, info.height, &vc_image_ptr
    );

    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);

    unsigned char *image = malloc(pitch * info.height);
    if (!image)
        die("cannot allocate image buffer");

    unsigned int last_chksum = UINT32_MAX;
    unsigned int stuck_count = 0;

    int watchdog_interval = env_num("WATCHDOG_INTERVAL", 1, 60);
    int watchdog_count = env_num("WATCHDOG_COUNT", 1, 10);

    fprintf(stderr, "starting watchdog for screen %dx%d. %d second interval, %d trigger\n",
        info.width, info.height, watchdog_interval, watchdog_count);

    int status = 2;

    while (running) {
        vc_dispmanx_snapshot(display, resource, VC_IMAGE_ROT0);
        vc_dispmanx_resource_read_data(resource, &rect, image, info.width * 3);

        unsigned int r = 0, g = 0, b = 0;
        for (int y = 0; y < info.height; y++) {
            const unsigned char *base = image + y * pitch;
            for (int x = 0; x < info.width * 3; x += 3) {
                const unsigned char *ptr = base + x;
                r += *(ptr+0);
                g += *(ptr+1);
                b += *(ptr+2);
            }
        }

        unsigned int chksum = r ^ g ^ b;
        fprintf(stderr, "chksum: %ld\n", chksum);

        if (chksum == last_chksum) {
            stuck_count++;
            if (stuck_count >= watchdog_count) {
                fprintf(stderr, "stuck detected!\n");
                status = 0;
                break;
            }
        } else {
            last_chksum = chksum;
            stuck_count = 0;
        }
        sleep(watchdog_interval);
    }

    free(image);
    vc_dispmanx_resource_delete(resource);
    vc_dispmanx_display_close(display);

    fprintf(stderr, "That's all. Have a nice day\n");
    return status;
}
