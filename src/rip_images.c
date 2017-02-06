#include "psys/psys_types.h"
#include "util/util_img.h"
#include "util/write_bmp.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static void write_image(unsigned addr, const psys_byte *data, unsigned width, unsigned height)
{
    static const unsigned char palette[16][4] = {
        { 0x00, 0x00, 0x00, 0xff },
        { 0x00, 0x00, 0xb6, 0xff },
        { 0x00, 0x92, 0x00, 0xff },
        { 0x00, 0xb6, 0xb6, 0xff },
        { 0xb6, 0x00, 0x00, 0xff },
        { 0xb6, 0x00, 0xb6, 0xff },
        { 0x92, 0x6d, 0x00, 0xff },
        { 0xb6, 0xb6, 0xb6, 0xff },
        { 0x49, 0x49, 0x49, 0xff },
        { 0x24, 0x49, 0xff, 0xff },
        { 0x00, 0xff, 0x00, 0xff },
        { 0x00, 0xff, 0xff, 0xff },
        { 0xff, 0x00, 0x00, 0xff },
        { 0xff, 0x00, 0xff, 0xff },
        { 0xff, 0xff, 0x00, 0xff },
        { 0xff, 0xff, 0xff, 0xff },
    };
    unsigned char *rgba = malloc(width * height * 4);
    char filename[80];
    unsigned xx, yy, ptr;

    sprintf(filename, "image_%05x.bmp", addr);
    ptr = 0;
    for (yy = 0; yy < height; ++yy) {
        for (xx = 0; xx < width; ++xx) {
            rgba[ptr * 4 + 0] = palette[data[ptr]][0];
            rgba[ptr * 4 + 1] = palette[data[ptr]][1];
            rgba[ptr * 4 + 2] = palette[data[ptr]][2];
            rgba[ptr * 4 + 3] = palette[data[ptr]][3];
            ptr++;
        }
    }
    bmp_dump32_ex((char *)rgba, width, height, true, false, false, filename);
    free(rgba);
    printf("Wrote image to %s\n", filename);
}

unsigned char workspace[65536];

static void rip_images(const char *imagename)
{
    int fd;
    psys_byte *disk_data;
    size_t disk_size, track_size;
    unsigned ptr;
    unsigned width, height;

    /* load disk image */
    track_size = 9 * 512;
    disk_size  = 80 * track_size;
    disk_data  = malloc(disk_size);
    fd         = open(imagename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "Could not open disk image %s\n", imagename);
        exit(1);
    }
    if (read(fd, disk_data, disk_size) < (ssize_t)disk_size) {
        perror("read");
        fprintf(stderr, "Could not read disk image\n");
        exit(1);
    }
    close(fd);

    /* decode all images */
    ptr = 0x0d800;
    while (true) {
        unsigned srcsize;
        width  = (disk_data[ptr] << 8) | disk_data[ptr + 1];
        height = (disk_data[ptr + 2] << 8) | disk_data[ptr + 3];

        if (width == 0 || height == 0) {
            break;
        }

        printf("0x%05x %dx%d\n", ptr, width, height);
        if (width * height > sizeof(workspace)) {
            printf("Too large to decode\n");
            exit(1);
        }
        util_img_decompress_image(workspace, &disk_data[ptr + 4], width, height, &srcsize);
        printf("0x%05x %dx%d 0x%x\n", ptr, width, height, srcsize + 4);
        write_image(ptr, workspace, width, height);

        ptr += srcsize + 4;
        ptr += (ptr & 1);

        if (ptr == 0x150fa) { /* jump over zeros */
            ptr = 0x15170;
        }
        if (ptr == 0x2bce4) { /* incomplete 32x12 image */
            ptr = 0x2bd14;
        }
    }
}

int main(int argc, char **argv)
{
    rip_images(argv[1]);
    return 0;
}
