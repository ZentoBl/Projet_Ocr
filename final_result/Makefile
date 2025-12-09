CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm -lSDL2 -lSDL2_image

SRC = main.c letter_parser.c
OBJ=$(SRC:.c=.o)

all: ocr_draw

ocr_draw: $(OBJ)
	gcc $(OBJ) -o ocr_draw $(LDFLAGS)

run: all
	rm -f solver_output.txt
	touch solver_output.txt
	
	while read W; do \
		../Solver/solver ../Letter_txt_creator/grid.txt "$$W" >> solver_output.txt; \
	done < ../Letter_txt_creator/word.txt

	./ocr_draw \
		../image_modifier/tests_images/level_1_image_1.png \
		../image_modifier/images_grid_letters/ \
		../image_modifier/images_word_letters/ \
		solver_output.txt \
		annotated.png

clean:
	rm -f *.o ocr_draw
	rm -f solver_output.txt annotated.png
