CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm -lSDL2 -lSDL2_image
SOLVEUR_OUT = solver_output.txt
SRC = grid_annotator.c letter_parser.c
OBJ=$(SRC:.c=.o)

all: ocr_draw

ocr_draw: $(OBJ)
	gcc $(OBJ) -o ocr_draw $(LDFLAGS)

run: all
	rm -f $(SOLVEUR_OUT)
	touch $(SOLVEUR_OUT)
	
	while read W; do \
		../Solver/solver \
		../Letter_txt_creator/grid.txt "$$W" >> $(SOLVEUR_OUT); \
	done < ../Letter_txt_creator/word.txt

	./ocr_draw \
		original_image_rotated.bmp\
		../image_modifier/images_grid_letters/ \
		../image_modifier/images_word_letters/ \
		$(SOLVEUR_OUT) \
		annotated.png

clean:cleanimg
	rm -f *.o ocr_draw
	

cleanimg:
	rm -f $(SOLVEUR_OUT) annotated.png
	rm -f original_image_rotated.bmp