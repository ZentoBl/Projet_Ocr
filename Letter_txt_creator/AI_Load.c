#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <SDL2/SDL.h>
#include <dirent.h>
#include <string.h>

#define INPUT_WIDTH 45
#define INPUT_HEIGHT 45
#define INPUT_SIZE (INPUT_WIDTH * INPUT_HEIGHT)
#define HIDDEN_SIZE 150
#define OUTPUT_SIZE 26
#define CONFIDENCE_THRESHOLD 0.70
#define MAX_RECURSIVE_DEPTH 2
#define OUTPUT_DIR "decoupage"

double weights_ih[INPUT_SIZE][HIDDEN_SIZE];
double biases_h[HIDDEN_SIZE];
double weights_ho[HIDDEN_SIZE][OUTPUT_SIZE];
double biases_o[OUTPUT_SIZE];

double hidden_outputs[HIDDEN_SIZE];
double output_outputs[OUTPUT_SIZE];

typedef struct {
    char character;
    double confidence;
} PredictionResult;

PredictionResult simple_recognition(SDL_Surface* surface);
char* recognize_recursive(SDL_Surface* surface, int depth, 
                         const char* prefix);
int* split_pos(SDL_Surface *img, size_t nb_cut);
SDL_Surface** split_surface_at_positions(SDL_Surface* surface, 
                                        int* positions, int nb_cuts);
void create_output_directory();
void save_surface(SDL_Surface* surf, const char* prefix, int part_num, 
                 char predicted);
double* convert_surface_to_matrix(SDL_Surface* surface);
int load_network(const char *filename);
char recognize(SDL_Surface* surface);

char* recognize_image(const char* model_path, const char* image_path) {
    static int network_loaded = 0;
    static char loaded_model[512] = "";
    
    if (!network_loaded || strcmp(loaded_model, model_path) != 0) {
        if (load_network(model_path) != 0) {
            fprintf(stderr, "Erreur: Impossible load model '%s'\n",
                    model_path);
            return NULL;
        }
        strncpy(loaded_model, model_path, sizeof(loaded_model) - 1);
        network_loaded = 1;
    }
    
    SDL_Surface *image_surface = SDL_LoadBMP(image_path);
    if (!image_surface) {
        fprintf(stderr, "Erreur: Impossile load image '%s': %s\n",
                image_path, SDL_GetError());
        return NULL;
    }
    
    if (image_surface->w > INPUT_WIDTH+2 || image_surface->h > INPUT_HEIGHT+2)
    {
        SDL_FreeSurface(image_surface);
        return "LAA";
    }

    char result = recognize(image_surface);
    char* result2 = malloc(2);
    result2[0]= result;
    result2[1]= '\0';
    SDL_FreeSurface(image_surface);
    
    return result2;
}

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double* convert_surface_to_matrix(SDL_Surface* surface) {
    int w = surface->w;
    int h = surface->h;

    /*
    if (w > INPUT_WIDTH || h > INPUT_HEIGHT) {
        fprintf(stderr, "Erreur: Image chargee (%dx%d) trop grande.\n", 
                w, h);
        return NULL;
    }
    */
    double* matrix = (double*)calloc(INPUT_SIZE, sizeof(double));
    if (!matrix) return NULL;

    int offset_x = (INPUT_WIDTH - w) / 2;
    int offset_y = (INPUT_HEIGHT - h) / 2;

    if (SDL_LockSurface(surface) != 0) {
        fprintf(stderr, "Erreur SDL verrouillage: %s\n", SDL_GetError());
        free(matrix);
        return NULL;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int nn_index = (y + offset_y) * INPUT_WIDTH + (x + offset_x);
            
            if (nn_index >= INPUT_SIZE) continue;

            Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch 
                       + x * surface->format->BytesPerPixel;
            Uint32 pixel = *(Uint32 *)p;

            Uint8 r, g, b, avg_color;
            SDL_GetRGB(pixel, surface->format, &r, &g, &b);
            avg_color = (r + g + b) / 3;

            if (avg_color < 128) {
                matrix[nn_index] = 1.0;
            }
        }
    }

    SDL_UnlockSurface(surface);
    return matrix;
}

void forward_pass(const double *input) {
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        double sum = biases_h[j];
        for (int i = 0; i < INPUT_SIZE; i++) {
            sum += input[i] * weights_ih[i][j];
        }
        hidden_outputs[j] = sigmoid(sum);
    }

    for (int k = 0; k < OUTPUT_SIZE; k++) {
        double sum = biases_o[k];
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            sum += hidden_outputs[j] * weights_ho[j][k];
        }
        output_outputs[k] = sigmoid(sum);
    }
}

char recognize(SDL_Surface* surface) 
{
    char result;
    
    double *user_input = convert_surface_to_matrix(surface);
    
    if (user_input == NULL) {
        return result;
    }

    forward_pass(user_input);
    
    double max_output = -1.0;
    int best_index = -1;
    
    for(int k = 0; k < OUTPUT_SIZE; k++) {
        if(output_outputs[k] > max_output) {
            max_output = output_outputs[k];
            best_index = k;
        }
    }
    
    free(user_input);

    if (best_index != -1) {
        result = (char)('A' + best_index);
    }
    
    return result;
}

PredictionResult simple_recognition(SDL_Surface* surface) {
    PredictionResult result = {'?', 0.0};
    
    double *user_input = convert_surface_to_matrix(surface);
    
    if (user_input == NULL) {
        return result;
    }

    forward_pass(user_input);
    
    double max_output = -1.0;
    int best_index = -1;
    
    for(int k = 0; k < OUTPUT_SIZE; k++) {
        if(output_outputs[k] > max_output) {
            max_output = output_outputs[k];
            best_index = k;
        }
    }
    
    free(user_input);

    if (best_index != -1) {
        result.character = (char)('A' + best_index);
        result.confidence = max_output;
    }
    
    return result;
}

void create_output_directory() {
#ifdef _WIN32
    _mkdir(OUTPUT_DIR);
#else
    mkdir(OUTPUT_DIR, 0755);
#endif
}

void save_surface(SDL_Surface* surf, const char* prefix, int part_num, 
                 char predicted) {
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s_partie%d_%c.bmp", 
             OUTPUT_DIR, prefix, part_num, predicted);
    
    SDL_SaveBMP(surf, output_path);
}

int* split_pos(SDL_Surface *img, size_t nb_cut) {
    if (!img || nb_cut == 0) return NULL;
    
    int bpp = img->format->BytesPerPixel;
    Uint8 *p = (Uint8*)img->pixels;
    
    int *hist = calloc(img->w, sizeof(int));
    if (!hist) err(EXIT_FAILURE, "split_pos calloc hist failed");
    
    for (int y = 0; y < img->h; y++) {
        Uint8 *row = p + y * img->pitch;
        for (int x = 0; x < img->w; x++) {
            Uint8 *px1 = row + x * bpp;
            if ((px1[0] + px1[1] + px1[2]) < 128 * 3) hist[x]++;
            
            if (y > 0 && y < img->h - 1) {
                Uint8 *px0 = px1 - img->pitch;
                Uint8 *px2 = px1 + img->pitch;
                if ((px0[0] + px0[1] + px0[2]) < 128 * 3 &&
                    (px2[0] + px2[1] + px2[2]) < 128 * 3)
                    hist[x] += 1;
            }
        }
    }
    
    int max_depth = 0;
    for (size_t x = 0; x < img->w; x++)
        if (hist[x] > max_depth)
            max_depth = hist[x];
    
    int valley_thresh = max_depth;
    for (size_t x = 2; x < img->w-2; x++)
        if (hist[x] < valley_thresh)
            valley_thresh = hist[x];
    
    int nb_at_min = 0;
    for (size_t x = 2; x < img->w-2; x++)
        if (valley_thresh == hist[x])
            nb_at_min++;
    
    valley_thresh = (valley_thresh > 1 ? valley_thresh : 1) + 1;
    if (nb_at_min <= nb_cut + 3)
        valley_thresh += 1;
    
    int *cands = malloc(sizeof(int) * img->w);
    size_t ccount = 0;
    char in_valley = 0;
    int vstart = 0;
    
    for (int x = 0; x < img->w; x++) {
        if (hist[x] <= valley_thresh) {
            if (!in_valley) {
                in_valley = 1;
                vstart = x;
            }
        } else if (in_valley) {
            int vend = x - 1;
            if (vstart > 2 && vend < img->w - 3)
                cands[ccount++] = (vstart + vend) / 2;
            in_valley = 0;
        }
    }
    
    if (in_valley) {
        int vend = img->w - 1;
        if (vstart > 2 && vend < img->w - 3)
            cands[ccount++] = (vstart + vend) / 2;
    }
    
    if (ccount < nb_cut) {
        free(hist);
        free(cands);
        return NULL;
    }
    
    if (ccount == 0) {
        free(hist);
        free(cands);
        return NULL;
    }
    
    double step = (double)img->w / (nb_cut + 1);
    int *splits = malloc(sizeof(int) * nb_cut);
    if (!splits) err(EXIT_FAILURE, "split_pos malloc split failed");
    
    for (size_t i = 0; i < nb_cut; i++) {
        double target = (i + 1) * step;
        int best = -1;
        double best_dist = 99999.0;
        
        for (size_t c = 0; c < ccount; c++) {
            double d = fabs((double)cands[c] - target);
            if (d < best_dist) {
                best_dist = d;
                best = cands[c];
            }
        }
        
        if (best < 0) errx(EXIT_FAILURE, "split_pos, best==-1");
        splits[i] = (int)((best + target) / 2);
    }
    
    free(hist);
    free(cands);
    return splits;
}

SDL_Surface** split_surface_at_positions(SDL_Surface* surface, 
                                        int* positions, int nb_cuts) {
    if (!surface || !positions || nb_cuts < 1) return NULL;
    
    int nb_parts = nb_cuts + 1;
    SDL_Surface** surfaces = malloc(nb_parts * sizeof(SDL_Surface*));
    if (!surfaces) return NULL;
    
    for (int i = 0; i < nb_parts; i++) {
        surfaces[i] = NULL;
    }
    
    int prev_x = 0;
    
    for (int i = 0; i < nb_parts; i++) {
        int next_x = (i < nb_cuts) ? positions[i] : surface->w;
        int part_width = next_x - prev_x;
        
        if (part_width <= 0) {
            for (int j = 0; j < i; j++) {
                if (surfaces[j]) SDL_FreeSurface(surfaces[j]);
            }
            free(surfaces);
            return NULL;
        }
        
        surfaces[i] = SDL_CreateRGBSurface(0, part_width, surface->h, 
                                          surface->format->BitsPerPixel,
                                          surface->format->Rmask, 
                                          surface->format->Gmask,
                                          surface->format->Bmask, 
                                          surface->format->Amask);
        
        if (!surfaces[i]) {
            for (int j = 0; j < i; j++) {
                if (surfaces[j]) SDL_FreeSurface(surfaces[j]);
            }
            free(surfaces);
            return NULL;
        }
        
        SDL_Rect src_rect = {prev_x, 0, part_width, surface->h};
        SDL_BlitSurface(surface, &src_rect, surfaces[i], NULL);
        
        prev_x = next_x;
    }
    
    return surfaces;
}

char* recognize_recursive(SDL_Surface* surface, int depth, 
                         const char* prefix) {
    static int global_part_counter = 0;
    
    if (depth > MAX_RECURSIVE_DEPTH) {
        return strdup("?");
    }
    
    if (surface->w > INPUT_WIDTH) {
        int estimated_letters = (surface->w + INPUT_WIDTH - 1) 
                               / INPUT_WIDTH;
        int nb_cuts = estimated_letters - 1;
        
        if (nb_cuts < 1) nb_cuts = 1;
        if (nb_cuts > 3) nb_cuts = 3;
        
        int* positions = split_pos(surface, nb_cuts);
        
        if (!positions) {
            return strdup("?");
        }
        
        SDL_Surface** parts = split_surface_at_positions(surface, positions, 
                                                        nb_cuts);
        free(positions);
        
        if (!parts) {
            return strdup("?");
        }
        
        int nb_parts = nb_cuts + 1;
        char** part_results = malloc(nb_parts * sizeof(char*));
        
        char new_prefix[256];
        for (int i = 0; i < nb_parts; i++) {
            snprintf(new_prefix, sizeof(new_prefix), "%s  [%d/%d] ", 
                    prefix, i+1, nb_parts);
            part_results[i] = recognize_recursive(parts[i], depth + 1, 
                                                 new_prefix);
        }
        
        size_t total_len = 1;
        for (int i = 0; i < nb_parts; i++) {
            if (part_results[i]) total_len += strlen(part_results[i]);
        }
        
        char* final_result = malloc(total_len);
        final_result[0] = '\0';
        
        for (int i = 0; i < nb_parts; i++) {
            if (part_results[i]) {
                strcat(final_result, part_results[i]);
                free(part_results[i]);
            }
        }
        free(part_results);
        
        for (int i = 0; i < nb_parts; i++) {
            SDL_FreeSurface(parts[i]);
        }
        free(parts);
        
        return final_result;
    }
    
    PredictionResult res = simple_recognition(surface);
    
    if (res.confidence >= CONFIDENCE_THRESHOLD) {
        char* result = malloc(2);
        result[0] = res.character;
        result[1] = '\0';
        
        save_surface(surface, "final", ++global_part_counter, 
                    res.character);
        
        return result;
    }
    
    for (int nb_cuts = 1; nb_cuts <= 5; nb_cuts++) {
        int nb_parts = nb_cuts + 1;
        
        int* positions = split_pos(surface, nb_cuts);
        
        if (!positions) {
            continue;
        }
        
        SDL_Surface** parts = split_surface_at_positions(surface, positions, 
                                                        nb_cuts);
        free(positions);
        
        if (!parts) {
            continue;
        }
        
        char** part_results = malloc(nb_parts * sizeof(char*));
        int all_success = 1;
        
        char new_prefix[256];
        for (int i = 0; i < nb_parts; i++) {
            snprintf(new_prefix, sizeof(new_prefix), "%s  [%d/%d] ", 
                    prefix, i+1, nb_parts);
            part_results[i] = recognize_recursive(parts[i], depth + 1, 
                                                 new_prefix);
            
            if (!part_results[i] || strcmp(part_results[i], "?") == 0) {
                all_success = 0;
            }
        }
        
        for (int i = 0; i < nb_parts; i++) {
            SDL_FreeSurface(parts[i]);
        }
        free(parts);
        
        if (all_success) {
            size_t total_len = 1;
            for (int i = 0; i < nb_parts; i++) {
                total_len += strlen(part_results[i]);
            }
            
            char* final_result = malloc(total_len);
            final_result[0] = '\0';
            
            for (int i = 0; i < nb_parts; i++) {
                strcat(final_result, part_results[i]);
                free(part_results[i]);
            }
            free(part_results);
            
            return final_result;
        }
        
        for (int i = 0; i < nb_parts; i++) {
            if (part_results[i]) free(part_results[i]);
        }
        free(part_results);
    }
    
    char* fallback = malloc(2);
    fallback[0] = res.character;
    fallback[1] = '\0';
    return fallback;
}

int load_network(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }

    char buffer[256];

    if (fgets(buffer, sizeof(buffer), file) == NULL || 
        strstr(buffer, "--- WEIGHTS_IH ---") == NULL) {
        fclose(file);
        return -1;
    }

    for (int i = 0; i < INPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            if (fscanf(file, "%lf", &weights_ih[i][j]) != 1) {
                fclose(file);
                return -1;
            }
        }
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) 
        fgets(buffer, sizeof(buffer), file);
    if (fgets(buffer, sizeof(buffer), file) == NULL || 
        strstr(buffer, "--- BIASES_H ---") == NULL) {
        fclose(file);
        return -1;
    }

    for (int j = 0; j < HIDDEN_SIZE; j++) {
        if (fscanf(file, "%lf", &biases_h[j]) != 1) {
            fclose(file);
            return -1;
        }
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) 
        fgets(buffer, sizeof(buffer), file);
    if (fgets(buffer, sizeof(buffer), file) == NULL || 
        strstr(buffer, "--- WEIGHTS_HO ---") == NULL) {
        fclose(file);
        return -1;
    }

    for (int j = 0; j < HIDDEN_SIZE; j++) {
        for (int k = 0; k < OUTPUT_SIZE; k++) {
            if (fscanf(file, "%lf", &weights_ho[j][k]) != 1) {
                fclose(file);
                return -1;
            }
        }
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) 
        fgets(buffer, sizeof(buffer), file);
    if (fgets(buffer, sizeof(buffer), file) == NULL || 
        strstr(buffer, "--- BIASES_O ---") == NULL) {
        fclose(file);
        return -1;
    }

    for (int k = 0; k < OUTPUT_SIZE; k++) {
        if (fscanf(file, "%lf", &biases_o[k]) != 1) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

