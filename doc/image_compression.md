Sundog image compression
---------------------------

(function GEMBIND:0x07)
Color images are compressed in an interesting way.
It looks like a RLE compression with a few stock patterns added for common cases.
Output images are represented by 4 image planes consisting of B/W images, these seem to be interleaved per word in a format

    <b0> <b1> <b2> <b3> <b0> <b1> ...

Where bX is 16 bits from plane X.

The input consists of a byte stream. The following operations patterns seem defined:

```
0xxxyyyy                            RLE (x+1) times y
10X0yyyy <data1h>                   RLE (data1+1) times y
11X0yyyy <data1h> <data1l>          RLE (data1+1) times y
1001yyyy <data1h> [data1*b]          data1 H4L4 encoded pixels follow, first pixel is y if len odd
1101yyyy <data1h> <data1l> [data1*b] data1 H4L4 encoded pixels follow, first pixel is y if len odd
1011yyyy <data1h>                   (data1+1) pixels from previous line
1111yyyy <data1h> <data1l>          (data1+1) pixels from previous line
```

The function writes output in units of 64 bits (4 words, one per plane).
These units are initialized to 0, then manipulated according to commands in the source stream.

This is implemented in `decompress_image(dst, src, width, height)` in `game_gembind.c` and
tested in `test_img.c`.

