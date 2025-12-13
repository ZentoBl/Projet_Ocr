#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include "../AI/AI_Load.c"

#define MAX_FILES 1000
#define MAX_FILENAME 256

typedef struct {
    int row;
    int col;
} Position;

// Version originale de la fonction extraire_position (sans inversion)
int extraire_position_original(const char *nom_fichier, Position *pos) {
    // Chercher ".png_"
    const char *p = strstr(nom_fichier, ".png_bw.bmp_");
    if (!p) return 0;
    
    p += strlen(".png_bw.bmp_");

    // Sauter l'id (nombre avant le prochain underscore)
    while (*p && *p != '_') p++;
    if (*p != '_') return 0;
    
    p++; // Sauter l'underscore
    
    printf("%s\n",p);
    const char *debut = nom_fichier;
    // Lire row x col
    int row, col;
    if (sscanf(p, "%dx%d_", &row, &col) == 2) {
        // Vérifier si le nom du fichier contient "level_2_image_1" avant ".png_bw.bmp_"
        if (strstr(debut, "level_2_image_1") != NULL) {
            // Inverser row et col
            pos->row = col;
            pos->col = row;
        } else {
            pos->row = row;
            pos->col = col;
        }
        return 1;
    }
    
    return 0;
}

// Nouvelle version avec inversion conditionnelle pour level_2_image_1
int extraire_position_word(const char *nom_fichier, Position *pos) {
    // Chercher ".png_"
    const char *p = strstr(nom_fichier, ".png_bw.bmp_");
    if (!p) return 0;
    
    
    p += strlen(".png_bw.bmp_");

    // Sauter l'id (nombre avant le prochain underscore)
    while (*p && *p != '_') p++;
    if (*p != '_') return 0;
    
    p++; // Sauter l'underscore
    
    printf("%s\n",p);

    // Lire row x col
    int row, col;
    if (sscanf(p, "%dx%d_", &row, &col) == 2) {
        // Vérifier si le nom du fichier contient "level_2_image_1" avant ".png_bw.bmp_"
        
            pos->row = row;
            pos->col = col;
        
        return 1;
    }
    
    return 0;
}

// Pointeur de fonction global pour choisir quelle version utiliser
int (*extraire_position)(const char *, Position *) = extraire_position_original;

// Fonction de comparaison pour qsort
int comparer_positions(const void *a, const void *b) {
    Position *pa = (Position *)a;
    Position *pb = (Position *)b;
    
    if (pa->row != pb->row)
        return pa->row - pb->row;  // Trier par row d'abord
    return pa->col - pb->col;      // Puis par col
}

// Structure pour stocker position et résultat de reconnaissance
typedef struct {
    Position pos;
    char *result;
    char chemin_image[512];
} ImageData;

// Fonction récursive pour parcourir les sous-dossiers
void parcourir_dossiers(const char *chemin, ImageData *images, int *nb_images) {
    DIR *dir;
    struct dirent *entry;
    char chemin_complet[512];
    
    dir = opendir(chemin);
    if (!dir) {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL && *nb_images < MAX_FILES) {
        // Ignorer . et ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        snprintf(chemin_complet, sizeof(chemin_complet), "%s/%s", chemin, entry->d_name);
        
        // Si c'est un dossier, parcourir récursivement
        if (entry->d_type == DT_DIR) {
            parcourir_dossiers(chemin_complet, images, nb_images);
        }
        // Si c'est un fichier .bmp, extraire la position et reconnaître l'image
        else if (strstr(entry->d_name, ".bmp") != NULL) {
            Position pos;
            if (extraire_position(entry->d_name, &pos)) {
                images[*nb_images].pos = pos;
                snprintf(images[*nb_images].chemin_image, sizeof(images[*nb_images].chemin_image), "%s", chemin_complet);
                images[*nb_images].result = NULL;
                (*nb_images)++;
                printf("Trouvé: %s -> row=%d, col=%d\n", entry->d_name, pos.row, pos.col);
            }
        }
    }
    closedir(dir);
}

// Fonction principale
void creer_grille(const char *repertoire, const char *fichier_sortie) {
    ImageData images[MAX_FILES];
    int nb_images = 0;
    
    // Parcourir récursivement tous les sous-dossiers
    parcourir_dossiers(repertoire, images, &nb_images);
    
    printf("Nombre de fichiers trouvés: %d\n", nb_images);
    
    if (nb_images == 0) {
        printf("Aucune position trouvée.\n");
        return;
    }
    
    // Trier les positions
    qsort(images, nb_images, sizeof(ImageData), comparer_positions);
    
    // Initialiser SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erreur: impossible d'initialiser SDL\n");
        return;
    }
    
    // Reconnaître toutes les images
    printf("\nRéconnaissance des images en cours...\n");
    for (int i = 0; i < nb_images; i++) {
        images[i].result = recognize_image("../AI/save", images[i].chemin_image);
        if (images[i].result) {
            printf("Image %d: %s -> Résultat: %s\n", i, images[i].chemin_image, images[i].result);
        } else {
            printf("Image %d: %s -> Erreur de reconnaissance\n", i, images[i].chemin_image);
        }
    }
    
    SDL_Quit();
    
    // Trouver les dimensions de la grille et afficher les statistiques
    int max_row = 0, max_col = 0;
    int min_row = 999999, min_col = 999999;
    for (int i = 0; i < nb_images; i++) {
        if (images[i].pos.row > max_row) max_row = images[i].pos.row;
        if (images[i].pos.col > max_col) max_col = images[i].pos.col;
        if (images[i].pos.row < min_row) min_row = images[i].pos.row;
        if (images[i].pos.col < min_col) min_col = images[i].pos.col;
    }
    
    printf("Statistiques:\n");
    printf("  Row: min=%d, max=%d\n", min_row, max_row);
    printf("  Col: min=%d, max=%d\n", min_col, max_col);
    
    int nb_rows = max_row - min_row + 1;
    int nb_cols = max_col - min_col + 1;
    
    printf("Dimensions de la grille: %d lignes x %d colonnes\n", nb_cols, nb_rows);
    printf("Positions attendues: %d (fichiers trouvés: %d)\n", nb_cols * nb_rows, nb_images);
    
    // Créer la grille
    char **grille = malloc(nb_cols * sizeof(char *));
    for (int i = 0; i < nb_cols; i++) {
        grille[i] = malloc((nb_rows + 1) * sizeof(char));
        memset(grille[i], ' ', nb_rows);
        grille[i][nb_rows] = '\0';
    }
    
    // Placer les résultats de reconnaissance aux positions (en normalisant avec min_row et min_col)
    for (int i = 0; i < nb_images; i++) {
        int r = images[i].pos.row - min_row;
        int c = images[i].pos.col - min_col;
        if (images[i].result && images[i].result[0] != '\0') {
            // Placer tous les caractères du résultat horizontalement
            for (int j = 0; images[i].result[j] != '\0' && (r + j) < nb_rows; j++) {
                grille[c][r + j] = images[i].result[j];
            }
        } else {
            // Si pas de résultat, mettre un '?'
            grille[c][r] = '?';
        }
    }
    
    // Écrire dans le fichier
    FILE *f = fopen(fichier_sortie, "w");
    if (!f) {
        printf("Erreur: impossible de créer le fichier %s\n", fichier_sortie);
        return;
    }
    
    // Écrire la grille
    for (int i = 0; i < nb_cols; i++) {
        fprintf(f, "%s\n", grille[i]);
    }
    
    fclose(f);
    
    printf("Grille créée dans %s\n", fichier_sortie);
    
    // Libérer la mémoire
    for (int i = 0; i < nb_images; i++) {
        if (images[i].result) {
            free(images[i].result);
        }
    }
    for (int i = 0; i < nb_cols; i++) {
        free(grille[i]);
    }
    free(grille);
}

int main(int argc, char *argv[]) {
    const char *repertoire = ".";
    const char *fichier_sortie = "grille.txt";
    
    if (argc > 1) repertoire = argv[1];
    if (argc > 2) fichier_sortie = argv[2];
    
    // Vérifier si argv[1] contient "word"
    if (argc > 1 && strstr(argv[1], "images_word_letters") != NULL) {
        extraire_position = extraire_position_word;
        printf("Mode: inversion pour level_2_image_1\n");
    } else {
        extraire_position = extraire_position_original;
        printf("Mode: sans inversion\n");
    }
    
    printf("Analyse du répertoire: %s\n", repertoire);
    creer_grille(repertoire, fichier_sortie);
    
    return 0;
}