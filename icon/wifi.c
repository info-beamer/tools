#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <bcm_host.h>

#include "icon.c"

void die(const char *err) {
    printf("ERR: %s\n", err);
    exit(1);
}

int opt(const char *name, int default_val) {
    int val = default_val;
    const char *str;
    if ((str = getenv(name))) {
        char *endptr = NULL;
        val = strtol(str, &endptr, 10);
        if (str[0] == '\0' || *endptr != '\0')
            die("invalid option value");
    }
    return val;
}

int main(void) {
    VC_IMAGE_TYPE_T type = VC_IMAGE_ARGB8888;

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);

    uint32_t screen_w, screen_h;
    if (graphics_get_display_size(display, &screen_w, &screen_h) < 0)
        die("cannot get resolution");

    DISPMANX_MODEINFO_T info;
    vc_dispmanx_display_get_info(display, &info);

    unsigned char image[ICON.width * ICON.height * ICON.bytes_per_pixel];
    ICON_RUN_LENGTH_DECODE(
        image, 
        ICON.rle_pixel_data, 
        ICON.width * ICON.height,
        ICON.bytes_per_pixel
    );

    for (int p = 0; p < ICON.width * ICON.height*4; p+=4) {
        unsigned char r, g, b;
        r = image[p+0];
        g = image[p+1];
        b = image[p+2];
        image[p+0] = b;
        image[p+1] = g;
        image[p+2] = r;
    }

    uint32_t vc_image_ptr;
    DISPMANX_RESOURCE_HANDLE_T resource = vc_dispmanx_resource_create(
        type, ICON.width, ICON.height, &vc_image_ptr
    );

    VC_RECT_T src_rect;
    VC_RECT_T dst_rect;
    vc_dispmanx_rect_set(&dst_rect, 0, 0, ICON.width, ICON.height);
    vc_dispmanx_resource_write_data(
        resource, type, ICON.width*4, image, &dst_rect
    );

    int target_x = opt("OFFSET_X", 10);
    int target_y = opt("OFFSET_Y", 10);

    if (target_x < 0)
        target_x += screen_w - ICON.width;

    if (target_y < 0)
        target_y += screen_h - ICON.height;

    vc_dispmanx_rect_set(&src_rect, 0, 0, ICON.width << 16, ICON.height << 16);
    vc_dispmanx_rect_set(&dst_rect, target_x, target_y, ICON.width, ICON.height);

    while (1) {
        DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(10);
        DISPMANX_ELEMENT_HANDLE_T element = vc_dispmanx_element_add(
            update, display, 
            2000,
            &dst_rect,
            resource,
            &src_rect,
            DISPMANX_PROTECTION_NONE,
            NULL,
            NULL,
            VC_IMAGE_ROT0
        );
        vc_dispmanx_update_submit_sync(update);

        sleep(1);

        update = vc_dispmanx_update_start(10);
        vc_dispmanx_element_remove(update, element);
        vc_dispmanx_update_submit_sync(update);

        sleep(1);
    }
    return 0;
}

