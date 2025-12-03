CFLAGS = `pkg-config --cflags --libs MagickWand` -O3 -Wall -g
LIBS = `pkg-config --libs gtk+-3.0` -lm
SRC = rotate_bmp_im.c
TARGET := rotate_bmp_im

.PHONY: all clean

$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LIBS)



all: $(TARGET)


clean:
	rm -f $(TARGET)
	rm -f final_rotated.bmp temp_input.bmp
