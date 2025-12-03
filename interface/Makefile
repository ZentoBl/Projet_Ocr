CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -g
LIBS = `pkg-config --libs gtk+-3.0` -lm
SRC = interface.c
TARGET := interface

.PHONY: all clean subdirs

$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LIBS)

subdirs:
	$(MAKE) -C automatic_rotation

all: subdirs $(TARGET)

clean:
	rm -f $(TARGET)
	$(MAKE) -C automatic_rotation clean
