CFLAGS := -Wall -Wextra -g
#SANITYFLAGS := -fsanitize=address -fsanitize-recover=address
LDFLAGS := -lm -lSDL2 -lSDL2_image

PREPROCESS := preprocess
GETLETTERS := get_letters

all: $(PREPROCESS) $(GETLETTERS)

$(PREPROCESS): base_img.c preprocess.c
	gcc base_img.c preprocess.c -o $@ $(CFLAGS) $(SANITYFLAGS) $(LDFLAGS)

$(GETLETTERS): base_img.c get_letters.c
	gcc base_img.c get_letters.c -o $@ $(CFLAGS) $(SANITYFLAGS) $(LDFLAGS)

run: cleanimg all
	./$(PREPROCESS) level_1_image_1.png
	./$(GETLETTERS) level_1_image_1.png_bw.bmp
	./$(PREPROCESS) level_1_image_2.png
	./$(GETLETTERS) level_1_image_2.png_bw.bmp
#	./$(PREPROCESS) level_2_image_1.png
#	./$(GETLETTERS) level_2_image_1.png_bw.bmp
#	./$(PREPROCESS) level_2_image_2.png
#	./$(GETLETTERS) level_2_image_2.png_bw.bmp
	./$(PREPROCESS) level_3_image_1.png
	./$(GETLETTERS) level_3_image_1.png_bw.bmp
	./$(PREPROCESS) level_3_image_2.png
	./$(GETLETTERS) level_3_image_2.png_bw.bmp

clean: cleanimg
	rm -f $(PREPROCESS) $(GETLETTERS)

cleanimg:
	rm -rf black_and_white/
	rm -rf images_boxes/ images_grid_letters/ images_word_letters/
	rm -rf debug/
	rm -f tests_images/gui_input.png tests_images/gui_input_rotated.png
	rm -f tests_images/gui_processed* tests_images/gui_rotated*
