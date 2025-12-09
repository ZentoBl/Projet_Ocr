CFLAGS := -Wall -Wextra -g
TARGET := Create_Grid_txt
SRC := Create_Grid_txt.c

$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) 

all: $(TARGET)

run: all
	./Create_Grid_txt \
		../image_modifier/images_grid_letters/ \
		../Letter_txt_creator/grid.txt
	./Create_Grid_txt \
		../image_modifier/images_word_letters/ \
		../Letter_txt_creator/word.txt

clean:
	rm -f $(TARGET)
