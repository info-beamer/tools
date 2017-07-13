// A simple demo using dispmanx to display an overlay

#include <stdlib.h>
#include <unistd.h>
#include <bcm_host.h>

#include "icon.c"

#define WIDTH   32
#define HEIGHT  32

int main(void) {
    VC_IMAGE_TYPE_T type = VC_IMAGE_ARGB8888;
    int width = WIDTH, height = HEIGHT;

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);

    DISPMANX_MODEINFO_T info;
    vc_dispmanx_display_get_info(display, &info);

    unsigned char image[ICON.width * ICON.height * ICON.bytes_per_pixel];
    ICON_RUN_LENGTH_DECODE(
        image, 
        ICON.rle_pixel_data, 
        ICON.width * ICON.height,
        ICON.bytes_per_pixel
    );

    for (int p = 0; p < width * height*4; p+=4) {
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
        type, width, height, &vc_image_ptr
    );

    VC_RECT_T src_rect;
    VC_RECT_T dst_rect;
    vc_dispmanx_rect_set(&dst_rect, 0, 0, width, height);
    vc_dispmanx_resource_write_data(
        resource, type, width*4, image, &dst_rect
    );

    vc_dispmanx_rect_set(&src_rect, 0, 0, width << 16, height << 16);
    vc_dispmanx_rect_set(&dst_rect, 10, 10, width, height);

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

    // vc_dispmanx_resource_delete(resource);
    // vc_dispmanx_display_close(display);
    return 0;
}

