CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -g
LIBS = `pkg-config --libs gtk+-3.0` 
SRC = interface.c
OBJ = $(SRC:.c=.o)
CC = gcc
all: app

app: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o app
