CFLAGS = -Wall -Wextra -g
LDFLAGS = -lSDL2 -lm

TARGETS = create_grid

.PHONY: all clean

all: $(TARGETS)

create_grid: Create_Grid_txt.c
	gcc $(CFLAGS) Create_Grid_txt.c -o $@ $(LDFLAGS)


clean: cleanimg
	rm -f $(TARGETS)
	

cleanimg:
	rm -f grid.txt word.txt