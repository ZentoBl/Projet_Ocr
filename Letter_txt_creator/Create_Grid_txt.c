#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include "AI_Load.c"

#define MAX_FILES 1000
#define MAX_FILENAME 256

typedef struct {
    int row;
    int col;
} Position;

// Original version of the extract_position function 
// (without inversion)
int extract_position_original(const char *filename, Position *pos) 
{
    // Search for ".png_"
    const char *p = strstr(filename, ".png_bw.bmp_");
    if (!p) return 0;
    
    p += strlen(".png_bw.bmp_");

    // Skip the id (number before the next underscore)
    while (*p && *p != '_') p++;
    if (*p != '_') return 0;
    
    p++; // Skip the underscore
    
    printf("%s\n",p);
    const char *start = filename;
    // Read row x col
    int row, col;
    if (sscanf(p, "%dx%d_", &row, &col) == 2) 
    {
        // Check if the filename contains "level_2_image_1" 
        // before ".png_bw.bmp_"
        if (strstr(start, "level_2_image_1") != NULL) 
        {
            // Swap row and col
            pos->row = col;
            pos->col = row;
        } else 
        {
            pos->row = row;
            pos->col = col;
        }
        return 1;
    }
    
    return 0;
}

// New version with conditional inversion for level_2_image_1
int extract_position_word(const char *filename, Position *pos) 
{
    // Search for ".png_"
    const char *p = strstr(filename, ".png_bw.bmp_");
    if (!p) return 0;
    
    
    p += strlen(".png_bw.bmp_");

    // Skip the id (number before the next underscore)
    while (*p && *p != '_') p++;
    if (*p != '_') return 0;
    
    p++; // Skip the underscore
    
    printf("%s\n",p);

    // Read row x col
    int row, col;
    if (sscanf(p, "%dx%d_", &row, &col) == 2) 
    {
        // Check if the filename contains "level_2_image_1" 
        // before ".png_bw.bmp_"
        
            pos->row = row;
            pos->col = col;
        
        return 1;
    }
    
    return 0;
}

// Global function pointer to choose which version to use
int (*extract_position)(const char *, Position *) = 
    extract_position_original;

// Comparison function for qsort
int compare_positions(const void *a, const void *b) 
{
    Position *pa = (Position *)a;
    Position *pb = (Position *)b;
    
    if (pa->row != pb->row)
        return pa->row - pb->row;  // Sort by row first
    return pa->col - pb->col;      // Then by col
}

// Structure to store position and recognition result
typedef struct 
{
    Position pos;
    char *result;
    char image_path[512];
} ImageData;

// Recursive function to traverse subdirectories
void traverse_directories(const char *path, ImageData *images, 
    int *nb_images)
{
    DIR *dir;
    struct dirent *entry;
    char full_path[512];
    
    dir = opendir(path);
    if (!dir) 
    {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL && *nb_images < MAX_FILES) 
    {
        // Ignore . and ..
        if (strcmp(entry->d_name, ".") == 0 || 
        strcmp(entry->d_name, "..") == 0)
            continue;
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, 
        entry->d_name);
        
        // If it's a directory, traverse recursively
        if (entry->d_type == DT_DIR) 
        {
            traverse_directories(full_path, images, nb_images);
        }
        // If it's a .bmp file, extract position and recognize the image
        else if (strstr(entry->d_name, ".bmp") != NULL) 
        {
            Position pos;
            if (extract_position(entry->d_name, &pos)) 
            {
                images[*nb_images].pos = pos;
                snprintf(images[*nb_images].image_path, 
                    sizeof(images[*nb_images].image_path), "%s", 
                    full_path);
                images[*nb_images].result = NULL;
                (*nb_images)++;
                printf("Found: %s -> row=%d, col=%d\n", 
                    entry->d_name, pos.row, pos.col);
            }
        }
    }
    closedir(dir);
}

// Main function
void create_grid(const char *directory, const char *output_file) 
{
    ImageData images[MAX_FILES];
    int nb_images = 0;
    
    // Recursively traverse all subdirectories
    traverse_directories(directory, images, &nb_images);
    
    printf("Number of files found: %d\n", nb_images);
    
    if (nb_images == 0) 
    {
        printf("No positions found.\n");
        return;
    }
    
    // Sort positions
    qsort(images, nb_images, sizeof(ImageData), compare_positions);
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        printf("Error: unable to initialize SDL\n");
        return;
    }
    
    // Recognize all images
    printf("\nRecognizing images in progress...\n");
    for (int i = 0; i < nb_images; i++) 
    {
        images[i].result = recognize_image("save", images[i].image_path);
        if (images[i].result) 
        {
            printf("Image %d: %s -> Result: %s\n", i, images[i].image_path,
                images[i].result);
        } else 
        {
            printf("Image %d: %s -> Recognition error\n", i, 
                images[i].image_path);
        }
    }
    
    SDL_Quit();
    
    // Find grid dimensions and display statistics
    int max_row = 0, max_col = 0;
    int min_row = 999999, min_col = 999999;
    for (int i = 0; i < nb_images; i++) 
    {
        if (images[i].pos.row > max_row) max_row = images[i].pos.row;
        if (images[i].pos.col > max_col) max_col = images[i].pos.col;
        if (images[i].pos.row < min_row) min_row = images[i].pos.row;
        if (images[i].pos.col < min_col) min_col = images[i].pos.col;
    }
    
    printf("Statistics:\n");
    printf("  Row: min=%d, max=%d\n", min_row, max_row);
    printf("  Col: min=%d, max=%d\n", min_col, max_col);
    
    int nb_rows = max_row - min_row + 1;
    int nb_cols = max_col - min_col + 1;
    
    printf("Grid dimensions: %d rows x %d columns\n", nb_cols, nb_rows);
    printf("Expected positions: %d (files found: %d)\n", 
        nb_cols * nb_rows, nb_images);
    
    // Create the grid
    char **grid = malloc(nb_cols * sizeof(char *));
    for (int i = 0; i < nb_cols; i++) 
    {
        grid[i] = malloc((nb_rows + 1) * sizeof(char));
        memset(grid[i], ' ', nb_rows);
        grid[i][nb_rows] = '\0';
    }
    
    // Place recognition results at positions 
    // (normalizing with min_row and min_col)
    for (int i = 0; i < nb_images; i++) 
    {
        int r = images[i].pos.row - min_row;
        int c = images[i].pos.col - min_col;
        if (images[i].result && images[i].result[0] != '\0') 
        {
            // Place all characters of the result horizontally
            for (int j = 0; images[i].result[j] != '\0' && 
                (r + j) < nb_rows; j++) 
            {
                grid[c][r + j] = images[i].result[j];
            }
        } else 
        {
            // If no result, put a '?'
            grid[c][r] = '?';
        }
    }
    
    // Write to file
    FILE *f = fopen(output_file, "w");
    if (!f) 
    {
        printf("Error: unable to create file %s\n", output_file);
        return;
    }
    
    // Write the grid
    for (int i = 0; i < nb_cols; i++) 
    {
        fprintf(f, "%s\n", grid[i]);
    }
    
    fclose(f);
    
    printf("Grid created in %s\n", output_file);
    
    // Free memory
    for (int i = 0; i < nb_images; i++) 
    {
        if (images[i].result) 
        {
            free(images[i].result);
        }
    }
    for (int i = 0; i < nb_cols; i++) 
    {
        free(grid[i]);
    }
    free(grid);
}

int main(int argc, char *argv[]) 
{
    const char *directory = ".";
    const char *output_file = "grille.txt";
    
    if (argc > 1) directory = argv[1];
    if (argc > 2) output_file = argv[2];
    
    // Check if argv[1] contains "word"
    if (argc > 1 && strstr(argv[1], "images_word_letters") != NULL) 
    {
        extract_position = extract_position_word;
        printf("Mode: inversion for level_2_image_1\n");
    } else 
    {
        extract_position = extract_position_original;
        printf("Mode: without inversion\n");
    }
    
    printf("Analyzing directory: %s\n", directory);
    create_grid(directory, output_file);
    
    return 0;
}