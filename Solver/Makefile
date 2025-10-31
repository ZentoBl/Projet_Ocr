CFLAGS := -Wall -Wextra -g
TARGET := solver
SRC := solver.c


$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) 

all: $(TARGET)


clean:
	rm -f $(TARGET)