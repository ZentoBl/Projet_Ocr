CFLAGS = -Wall -Wextra -g
#MAGICK_CFLAGS = `pkg-config --cflags `


TARGETS = create_grid

.PHONY: all clean

all: $(TARGETS)

create_grid: Create_Grid_txt.c
	gcc $(CFLAGS) Create_Grid_txt.c -o $@


clean:
	rm -f $(TARGETS)
	rm -f grille.txt
