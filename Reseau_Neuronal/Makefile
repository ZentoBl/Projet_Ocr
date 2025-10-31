CFLAGS := -Wall -Wextra -g
LDFLAGS := -lm 
TARGET := fonction_XNOR
SRC := fonction_XNOR.c


$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LDFLAGS)

all: $(TARGET)


clean:
	rm -f $(TARGET)