#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rfb/rfb.h>

#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include "bcm_host.h"
#include <assert.h>

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

#define BPP 2

#define PICTURE_TIMEOUT (1.0/15.0)

static DISPMANX_DISPLAY_HANDLE_T display;
static DISPMANX_RESOURCE_HANDLE_T resource;
static DISPMANX_MODEINFO_T info;
static void *image;
static void *back_image;

static int padded_width;
static int pitch;
static int r_x0, r_y0, r_x1, r_y1;

static int done = 0;

static int time_for_next_picture() {
    static struct timeval now = { 0, 0 }, then = { 0, 0};
    double elapsed, dnow, dthen;

    gettimeofday(&now, NULL);

    dnow = now.tv_sec + (now.tv_usec / 1000000.0);
    dthen = then.tv_sec + (then.tv_usec / 1000000.0);
    elapsed = dnow - dthen;

    if (elapsed > PICTURE_TIMEOUT)
        memcpy((char *)&then, (char *)&now, sizeof(struct timeval));
    return elapsed > PICTURE_TIMEOUT;
}

static int take_picture(unsigned char *buffer) {
    int x, y;

    VC_IMAGE_TRANSFORM_T transform = 0;
    VC_RECT_T rect;

    vc_dispmanx_snapshot(display, resource, transform);

    vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
    vc_dispmanx_resource_read_data(resource, &rect, image, info.width * 2);

    unsigned short *image_p = (unsigned short *)image;
    unsigned short *buffer_p = (unsigned short *)buffer;

    unsigned short *back_image_p = (unsigned short *)back_image;
    for (y = 0; y < info.height; y++) {
        for (x = 0; x < info.width; x++) {
            if (back_image_p[y*padded_width + x] != image_p[y*padded_width + x]) {
                r_y0 = y;
                goto found_y0;
            }
        }
    }

    /* all scanned without hitting any difference? -> no need to send */
    return 0;

found_y0:

    for (y = info.height - 1; y >= r_y0; y--) {
        for (x = 0; x < info.width; x++) {
            if (back_image_p[y*padded_width + x] != image_p[y*padded_width + x]) {
                r_y1 = y + 1;
                goto found_y1;
            }
        }
    }
found_y1:

    for (x = 0; x < info.width; x++) {
        for (y = r_y0; y < r_y1; y++) {
            if (back_image_p[y*padded_width + x] != image_p[y*padded_width + x]) {
                r_x0 = x;
                goto found_x0;
            }
        }
    }
found_x0:

    for (x = info.width - 1; x >= r_x0; x--) {
        for (y = r_y0; y < r_y1; y++) {
            if (back_image_p[y*padded_width + x] != image_p[y*padded_width + x]) {
                r_x1 = x + 1;
                goto found_x1;
            }
        }
    }
found_x1:

    for (y = r_y0; y < r_y1; ++y) {
        for (x = r_x0; x < r_x1; ++x) {
            unsigned short tbi = image_p[y * padded_width + x];

            unsigned short R5 = (tbi >> 11);
            unsigned short G5 = (tbi >>  6) & 0x1f;
            unsigned short B5 = (tbi      ) & 0x1f;

            tbi = (B5 << 10) | (G5 << 5) | R5;

            buffer_p[y * padded_width + x] = tbi;
        }
    }

    /* swap image and back_image buffers */
    void *tmp_image = back_image;
    back_image = image;
    image = tmp_image;

    // printf("Rect: %d,%d - %d,%d\n", r_x0, r_y0, r_x1, r_y1);
    return 1;
}

static void sig_handler(int signo) {
    done = 1;
}

int main(int argc, char *argv[]) {
    bcm_host_init();

    signal(SIGINT, sig_handler);

    VC_IMAGE_TYPE_T type = VC_IMAGE_RGB565;

    int ret, end;

    uint32_t screen = 0;

    printf("Open display[%i]...\n", screen);
    display = vc_dispmanx_display_open(screen);

    ret = vc_dispmanx_display_get_info(display, &info);
    assert(ret == 0);

    /* DispmanX expects buffer rows to be aligned to a 32 bit boundary */
    pitch = ALIGN_UP(2 * info.width, 32);
    padded_width = pitch / BPP;

    printf("Display is %d x %d\n", info.width, info.height);

    image = calloc(1, pitch * info.height);
    assert(image);

    back_image = calloc(1, pitch * info.height);
    assert(back_image);

    r_x0 = r_y0 = 0;
    r_x1 = info.width;
    r_y1 = info.height;

    uint32_t vc_image_ptr;
    resource = vc_dispmanx_resource_create(type, info.width, info.height, &vc_image_ptr);

    rfbScreenInfoPtr server = rfbGetScreen(&argc, argv, padded_width, info.height, 5, 3, BPP);
    assert(server);

    server->desktopName = "info-beamer VNC";
    server->frameBuffer = malloc(pitch * info.height);
    server->alwaysShared = 1;

    printf("Server bpp:%d\n", server->serverFormat.bitsPerPixel);
    printf("Server bigEndian:%d\n", server->serverFormat.bigEndian);
    printf("Server redShift:%d\n", server->serverFormat.redShift);
    printf("Server blueShift:%d\n", server->serverFormat.blueShift);
    printf("Server greeShift:%d\n", server->serverFormat.greenShift);

    /* Initialize the server */
    rfbInitServer(server);

    while (rfbIsActive(server) && !done) {
        if (time_for_next_picture() && take_picture((unsigned char *)server->frameBuffer))
            rfbMarkRectAsModified(server, r_x0, r_y0, r_x1, r_y1);

        long usec = server->deferUpdateTime * 1000;
        rfbProcessEvents(server, usec);
    }

 end:
    ret = vc_dispmanx_resource_delete(resource);
    assert(ret == 0);

    ret = vc_dispmanx_display_close(display);
    assert(ret == 0);

    printf("\nDone\n");
    return 0;
}
