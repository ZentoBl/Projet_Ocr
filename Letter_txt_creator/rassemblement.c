#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILES 1000
#define MAX_FILENAME 256

typedef struct {
    int x;
    int y;
} Coordonnees;

// Fonction pour extraire les coordonnées du nom de fichier
// Format: grid_letter_level_1_image_1.png_0_0x0_(200x30_10x17)
int extraire_coordonnees(const char *nom_fichier, Coordonnees *coord) {
    char *pos = strrchr(nom_fichier, '_');
    if (!pos) return 0;
    
    // Cherche le pattern: _XxY)
    int x, y;
    if (sscanf(pos, "_%dx%d)", &x, &y) == 2) {
        coord->x = x;
        coord->y = y;
        return 1;
    }
    return 0;
}

// Fonction de comparaison pour qsort
int comparer_coords(const void *a, const void *b) {
    Coordonnees *ca = (Coordonnees *)a;
    Coordonnees *cb = (Coordonnees *)b;
    
    if (ca->y != cb->y)
        return ca->y - cb->y;  // Trier par y d'abord
    return ca->x - cb->x;      // Puis par x
}

// Fonction principale
void creer_grille(const char *repertoire, const char *fichier_sortie) {
    DIR *dir;
    struct dirent *entry;
    Coordonnees coords[MAX_FILES];
    int nb_coords = 0;
    
    // Ouvrir le répertoire
    dir = opendir(repertoire);
    if (!dir) {
        printf("Erreur: impossible d'ouvrir le répertoire %s\n", repertoire);
        return;
    }
    
    // Lire tous les fichiers et extraire les coordonnées
    while ((entry = readdir(dir)) != NULL && nb_coords < MAX_FILES) {
        if (strstr(entry->d_name, ".png") != NULL) {
            Coordonnees coord;
            if (extraire_coordonnees(entry->d_name, &coord)) {
                coords[nb_coords++] = coord;
            }
        }
    }
    closedir(dir);
    
    printf("Nombre de fichiers trouvés: %d\n", nb_coords);
    
    if (nb_coords == 0) {
        printf("Aucune coordonnée trouvée.\n");
        return;
    }
    
    // Trier les coordonnées
    qsort(coords, nb_coords, sizeof(Coordonnees), comparer_coords);
    
    // Trouver les dimensions maximales
    int max_x = 0, max_y = 0;
    for (int i = 0; i < nb_coords; i++) {
        if (coords[i].x > max_x) max_x = coords[i].x;
        if (coords[i].y > max_y) max_y = coords[i].y;
    }
    
    // Créer la grille
    char **grille = malloc((max_y + 1) * sizeof(char *));
    for (int i = 0; i <= max_y; i++) {
        grille[i] = malloc((max_x + 2) * sizeof(char));
        memset(grille[i], ' ', max_x + 1);
        grille[i][max_x + 1] = '\0';
    }
    
    // Placer les X aux coordonnées
    for (int i = 0; i < nb_coords; i++) {
        grille[coords[i].y][coords[i].x] = 'X';
    }
    
    // Écrire dans le fichier
    FILE *f = fopen(fichier_sortie, "w");
    if (!f) {
        printf("Erreur: impossible de créer le fichier %s\n", fichier_sortie);
        return;
    }
    
    for (int i = 0; i <= max_y; i++) {
        fprintf(f, "%s\n", grille[i]);
    }
    fclose(f);
    
    printf("Grille créée dans %s (%dx%d)\n", fichier_sortie, max_x + 1, max_y + 1);
    
    // Libérer la mémoire
    for (int i = 0; i <= max_y; i++) {
        free(grille[i]);
    }
    free(grille);
}

int main(int argc, char *argv[]) {
    const char *repertoire = ".";
    const char *fichier_sortie = "grille.txt";
    
    if (argc > 1) repertoire = argv[1];
    if (argc > 2) fichier_sortie = argv[2];
    
    printf("Analyse du répertoire: %s\n", repertoire);
    creer_grille(repertoire, fichier_sortie);
    
    return 0;
}