CFLAGS = -O3 -Wall -g
MAGICK_CFLAGS = `pkg-config --cflags MagickWand`
MAGICK_LIBS = `pkg-config --libs MagickWand`

GTK_LIBS = `pkg-config --libs gtk+-3.0`

TARGETS = rotate_bmp_im

.PHONY: all clean

all: $(TARGETS)

rotate_bmp_im: rotate_bmp_im.c
	gcc $(CFLAGS) $(MAGICK_CFLAGS) rotate_bmp_im.c -o $@ $(MAGICK_LIBS) $(GTK_LIBS)


clean:
	rm -f $(TARGETS)
	rm -f final_rotated.bmp temp_input.bmp
