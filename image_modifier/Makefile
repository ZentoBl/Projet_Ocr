CFLAGS := -Wall -Wextra -g
#SANITYFLAGS := -fsanitize=address -fsanitize-recover=address
LDFLAGS := -lm -lSDL2 -lSDL2_image
TARGET := image_modifier
SRC := main.c base_img.c preprocess.c get_letters.c

$(TARGET): $(SRC)
	gcc $(SRC) -o $@ $(CFLAGS) $(LDFLAGS)

all: $(TARGET)

run: $(TARGET)
	./$(TARGET) level_1_image_1.png
	./$(TARGET) level_1_image_2.png
	./$(TARGET) level_2_image_1.png
	./$(TARGET) level_2_image_2.png
	./$(TARGET) level_3_image_1.png
	./$(TARGET) level_3_image_2.png

clean:
	rm -f $(TARGET)
