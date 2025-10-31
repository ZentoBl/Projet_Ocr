CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -g
LIBS = `pkg-config --libs gtk+-3.0` -lm
SRC = interface.c
TARGET := interface



$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LIBS)



all: $(TARGET)


clean:
	rm -f $(TARGET)
