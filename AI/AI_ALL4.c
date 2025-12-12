#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <SDL2/SDL.h>


#define INPUT_WIDTH 45
#define INPUT_HEIGHT 45
#define INPUT_SIZE (INPUT_WIDTH * INPUT_HEIGHT) 
#define HIDDEN_SIZE 150
#define OUTPUT_SIZE 26 
#define LEARNING_RATE 0.005
#define INIT_RANGE 0.1
#define PROGRESS_STEP 100


double weights_ih[INPUT_SIZE][HIDDEN_SIZE];
double biases_h[HIDDEN_SIZE];
double weights_ho[HIDDEN_SIZE][OUTPUT_SIZE];
double biases_o[OUTPUT_SIZE];

double hidden_outputs[HIDDEN_SIZE];
double output_outputs[OUTPUT_SIZE];


typedef struct
{
    double *input;
    char target_char;
} TrainingExample;

typedef struct
{
    TrainingExample *data;
    int size;
} DatasetCollection;


double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

double sigmoid_derivative(double x)
{
    return x * (1.0 - x);
}

void generate_target(char letter, double *target)
{
    int index = letter - 'A';
    for (int i = 0; i < OUTPUT_SIZE; i++)
    {
        target[i] = 0.0;
    }
    if (index >= 0 && index < OUTPUT_SIZE)
    {
        target[index] = 1.0;
    }
}

char get_predicted_letter(const double *outputs)
{
    double max_output = -1.0;
    int max_index = 0;

    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        if (outputs[k] > max_output)
        {
            max_output = outputs[k];
            max_index = k;
        }
    }
    return (char)('A' + max_index);
}


double *convert_surface_to_matrix(SDL_Surface *surface)
{
    int w = surface->w;
    int h = surface->h;

    if (w > INPUT_WIDTH || h > INPUT_HEIGHT)
    {
        fprintf(stderr, "Erreur: Image chargee (%dx%d) est trop grande."
            , w, h);
        fprintf(stderr, " Elle doit etre <= %dx%d.\n"
            , INPUT_WIDTH, INPUT_HEIGHT);
        return NULL;
    }

    double *matrix = (double *)calloc(INPUT_SIZE, sizeof(double));
    if (!matrix)
        return NULL;

    int offset_x = (INPUT_WIDTH - w) / 2;
    int offset_y = (INPUT_HEIGHT - h) / 2;

    if (SDL_LockSurface(surface) != 0)
    {
        fprintf(stderr, "Erreur SDL lors du verrouillage: %s\n"
            , SDL_GetError());
        free(matrix);
        return NULL;
    }

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int nn_index = (y + offset_y) * INPUT_WIDTH + (x + offset_x);

            if (nn_index >= INPUT_SIZE)
                continue;

            Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch +
                       x * surface->format->BytesPerPixel;
            Uint32 pixel = *(Uint32 *)p;

            Uint8 r, g, b, avg_color;
            SDL_GetRGB(pixel, surface->format, &r, &g, &b);
            avg_color = (r + g + b) / 3;

            if (avg_color < 128)
            {
                matrix[nn_index] = 1.0;
            }
        }
    }

    SDL_UnlockSurface(surface);
    return matrix;
}

DatasetCollection load_and_process_images(const char *dir_path)
{
    DIR *d;
    struct dirent *dir;
    TrainingExample *temp_dataset = NULL;
    int dataset_count = 0;

    d = opendir(dir_path);
    if (!d)
    {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le repertoire '%s'.\n",
                dir_path);
        return (DatasetCollection){NULL, 0};
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Erreur SDL: %s\n", SDL_GetError());
        closedir(d);
        return (DatasetCollection){NULL, 0};
    }

    while ((dir = readdir(d)) != NULL)
    {
        const char *filename = dir->d_name;
        size_t len = strlen(filename);

        if (len > 4 && strcmp(filename + len - 4, ".bmp") == 0)
        {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, filename);

            SDL_Surface *image_surface = SDL_LoadBMP(full_path);

            if (!image_surface)
            {
                fprintf(stderr, "Erreur de chargement BMP '%s': %s\n"
                    , full_path,
                        SDL_GetError());
                continue;
            }

            double *matrix = convert_surface_to_matrix(image_surface);

            if (matrix)
            {
                temp_dataset = (TrainingExample *)realloc(
                    temp_dataset, (dataset_count + 1) 
                    * sizeof(TrainingExample));

                temp_dataset[dataset_count].input = matrix;
                temp_dataset[dataset_count].target_char = filename[0];

                dataset_count++;
                printf("Charge : %s (Cible: %c) | Total : %d images\n"
                    , filename,
                       filename[0], dataset_count);
            }

            SDL_FreeSurface(image_surface);
        }
    }

    closedir(d);
    SDL_Quit();
    return (DatasetCollection){temp_dataset, dataset_count};
}



void initialize_network()
{
    srand(time(NULL));

    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            weights_ih[i][j] = ((double)rand() / RAND_MAX) 
            * (2.0 * INIT_RANGE) -
                               INIT_RANGE;
        }
    }

    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        biases_h[j] = ((double)rand() / RAND_MAX) * (2.0 * INIT_RANGE) -
                      INIT_RANGE;
    }

    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        for (int k = 0; k < OUTPUT_SIZE; k++)
        {
            weights_ho[j][k] = ((double)rand() / RAND_MAX) 
            * (2.0 * INIT_RANGE) -
                               INIT_RANGE;
        }
    }

    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        biases_o[k] = ((double)rand() / RAND_MAX) * (2.0 * INIT_RANGE) -
                      INIT_RANGE;
    }
}

void forward_pass(const double *input)
{

    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        double sum = biases_h[j];
        for (int i = 0; i < INPUT_SIZE; i++)
        {
            sum += input[i] * weights_ih[i][j];
        }
        hidden_outputs[j] = sigmoid(sum);
    }

    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        double sum = biases_o[k];
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            sum += hidden_outputs[j] * weights_ho[j][k];
        }
        output_outputs[k] = sigmoid(sum);
    }
}

void train_step(const double *input, const double *target)
{
    forward_pass(input);


    double output_deltas[OUTPUT_SIZE];
    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        double error = target[k] - output_outputs[k];
        output_deltas[k] = error * sigmoid_derivative(output_outputs[k]);
    }


    double hidden_deltas[HIDDEN_SIZE];
    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        double error_sum = 0.0;
        for (int k = 0; k < OUTPUT_SIZE; k++)
        {
            error_sum += output_deltas[k] * weights_ho[j][k];
        }
        hidden_deltas[j] = error_sum * sigmoid_derivative(hidden_outputs[j]);
    }


    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        for (int k = 0; k < OUTPUT_SIZE; k++)
        {
            weights_ho[j][k] += LEARNING_RATE * output_deltas[k] 
            * hidden_outputs[j];
        }
    }
    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        biases_o[k] += LEARNING_RATE * output_deltas[k];
    }

    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            weights_ih[i][j] += LEARNING_RATE * hidden_deltas[j] * input[i];
        }
    }
    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        biases_h[j] += LEARNING_RATE * hidden_deltas[j];
    }
}

int save_network(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier de sauvegarde");
        return -1;
    }

    fprintf(file, "--- WEIGHTS_IH ---\n");
    for (int i = 0; i < INPUT_SIZE; i++)
    {
        for (int j = 0; j < HIDDEN_SIZE; j++)
        {
            fprintf(file, "%.10f ", weights_ih[i][j]);
        }
        fprintf(file, "\n");
    }

    fprintf(file, "--- BIASES_H ---\n");
    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        fprintf(file, "%.10f ", biases_h[j]);
    }
    fprintf(file, "\n");

    fprintf(file, "--- WEIGHTS_HO ---\n");
    for (int j = 0; j < HIDDEN_SIZE; j++)
    {
        for (int k = 0; k < OUTPUT_SIZE; k++)
        {
            fprintf(file, "%.10f ", weights_ho[j][k]);
        }
        fprintf(file, "\n");
    }

    fprintf(file, "--- BIASES_O ---\n");
    for (int k = 0; k < OUTPUT_SIZE; k++)
    {
        fprintf(file, "%.10f ", biases_o[k]);
    }
    fprintf(file, "\n");

    fclose(file);
    return 0;
}


int main()
{
    int epochs;
    printf("Entrez le nombre de generations (epochs) d'entrainement: ");
    if (scanf("%d", &epochs) != 1 || epochs <= 0)
    {
        printf("Saisie invalide. Arret.\n");
        return 1;
    }

    DatasetCollection collection = load_and_process_images("Image_train");

    if (collection.size == 0)
    {
        fprintf(stderr, "Aucune image chargee. Entrainement annule.\n");
        return 1;
    }

    TrainingExample *dataset = collection.data;
    const int DATASET_SIZE = collection.size;

    initialize_network();

    printf("\n--- Debut de l'entrainement sur %d images %dx%d ---\n",
           DATASET_SIZE, INPUT_WIDTH, INPUT_HEIGHT);

    for (int epoch = 1; epoch <= epochs; epoch++)
    {
        for (int i = 0; i < DATASET_SIZE; i++)
        {
            double target_vec[OUTPUT_SIZE];
            generate_target(dataset[i].target_char, target_vec);
            train_step(dataset[i].input, target_vec);
        }

        if (epoch % PROGRESS_STEP == 0 || epoch == epochs)
        {
            int total_successful_predictions = 0;
            int count_per_unique_letter[OUTPUT_SIZE] = {0};
            int successful_count_per_unique_letter[OUTPUT_SIZE] = {0};

            for (int i = 0; i < DATASET_SIZE; i++)
            {
                forward_pass(dataset[i].input);

                char predicted_char = get_predicted_letter(output_outputs);
                int target_index = dataset[i].target_char - 'A';

                if (target_index >= 0 && target_index < OUTPUT_SIZE)
                {
                    count_per_unique_letter[target_index]++;

                    if (predicted_char == dataset[i].target_char)
                    {
                        total_successful_predictions++;
                        successful_count_per_unique_letter[target_index]++;
                    }
                }
            }

            double success_rate = (double)total_successful_predictions /
                                  DATASET_SIZE * 100.0;

            printf("\n--- Generation %d ---\n", epoch);
            printf("Taux de reussite global: **%.2f%%** (%d/%d)\n",
                   success_rate, total_successful_predictions, DATASET_SIZE);
            printf("--------------------------------------------------\n");
            printf("Statut detaille par Lettre (Succes/Total) :\n");

            for (int k = 0; k < OUTPUT_SIZE; k++)
            {
                char letter = (char)('A' + k);

                if (count_per_unique_letter[k] > 0)
                {
                    double letter_rate = 
                    (double)successful_count_per_unique_letter[k] /
                                         count_per_unique_letter[k] * 100.0;

                    printf("  %c : ", letter);

                    if (successful_count_per_unique_letter[k] ==
                        count_per_unique_letter[k])
                    {
                        printf("✅ SUCCES PARFAIT (100.00%%, %d/%d)\n",
                               successful_count_per_unique_letter[k],
                               count_per_unique_letter[k]);
                    }
                    else
                    {
                        printf("❌ ECHEC ou PARTIEL (%.2f%%, %d/%d)\n",
                               letter_rate,
                               successful_count_per_unique_letter[k],
                               count_per_unique_letter[k]);
                    }
                }
            }
            printf("--------------------------------------------------\n");
        }
    }

    printf("\n--- Entrainement termine ---\n");

    char save_choice[10];
    printf("\n\n------------------------------------------------\n");
    printf("Sauvegarder les parametres du reseau ?\n");
    printf("Saisissez 'O' (Oui) ou 'N' (Non) : ");

    if (scanf("%s", save_choice) == 1 &&
        (save_choice[0] == 'O' || save_choice[0] == 'o'))
    {
        printf("Nom du fichier de sauvegarde (ex: mon_modele.txt) : ");
        char filename[256];
        if (scanf("%s", filename) == 1)
        {
            if (save_network(filename) == 0)
            {
                printf("✅ Sauvegarde reussie dans le fichier '%s'!\n", filename);
            }
            else
            {
                printf("❌ La sauvegarde a echoue.\n");
            }
        }
    }
    else
    {
        printf("Sauvegarde annulee.\n");
    }

    for (int i = 0; i < DATASET_SIZE; i++)
    {
        free(dataset[i].input);
    }
    free(dataset);

    return 0;
}