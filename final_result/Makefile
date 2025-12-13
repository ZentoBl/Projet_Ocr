CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm -lSDL2 -lSDL2_image

SRC = grid_annotator.c letter_parser.c
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
		original_image_rotated.bmp\
		../image_modifier/images_grid_letters/ \
		../image_modifier/images_word_letters/ \
		solver_output.txt \
		annotated.png

clean:cleanimg
	rm -f *.o ocr_draw
	

cleanimg:
	rm -f solver_output.txt annotated.png
	rm -f original_image_rotated.bmp