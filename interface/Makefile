CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -g
LIBS = `pkg-config --libs gtk+-3.0` -lm
SRC = interface.c
TARGET := interface

.PHONY: all clean subdirs

all: subdirs $(TARGET)

$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LIBS)

subdirs:
	$(MAKE) -C automatic_rotation

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
	$(MAKE) -C automatic_rotation clean
