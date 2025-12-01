#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_FILES 1000
#define MAX_FILENAME 256

typedef struct {
    int row;
    int col;
} Position;

// Fonction pour extraire row et col du nom de fichier
// Format: grid_letter_level_1_image_1.png_{id}_{row}x{col}_(...)
// Exemple: grid_letter_level_1_image_1.png_100_15x5_(718x203_10x17).bmp
int extraire_position(const char *nom_fichier, Position *pos) {
    // Chercher ".png_"
    const char *p = strstr(nom_fichier, ".png_");
    if (!p) return 0;
    
    p += 5; // Sauter ".png_"
    
    // Sauter l'id (nombre avant le prochain underscore)
    while (*p && *p != '_') p++;
    if (*p != '_') return 0;
    
    p++; // Sauter l'underscore
    
    // Lire row x col
    int row, col;
    if (sscanf(p, "%dx%d_", &row, &col) == 2) {
        pos->row = row;
        pos->col = col;
        return 1;
    }
    
    return 0;
}

// Fonction de comparaison pour qsort
int comparer_positions(const void *a, const void *b) {
    Position *pa = (Position *)a;
    Position *pb = (Position *)b;
    
    if (pa->row != pb->row)
        return pa->row - pb->row;  // Trier par row d'abord
    return pa->col - pb->col;      // Puis par col
}

// Fonction récursive pour parcourir les sous-dossiers
void parcourir_dossiers(const char *chemin, Position *positions, int *nb_positions) {
    DIR *dir;
    struct dirent *entry;
    char chemin_complet[512];
    
    dir = opendir(chemin);
    if (!dir) {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL && *nb_positions < MAX_FILES) {
        // Ignorer . et ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        snprintf(chemin_complet, sizeof(chemin_complet), "%s/%s", chemin, entry->d_name);
        
        // Si c'est un dossier, parcourir récursivement
        if (entry->d_type == DT_DIR) {
            parcourir_dossiers(chemin_complet, positions, nb_positions);
        }
        // Si c'est un fichier .bmp, extraire la position
        else if (strstr(entry->d_name, ".bmp") != NULL) {
            Position pos;
            if (extraire_position(entry->d_name, &pos)) {
                positions[(*nb_positions)++] = pos;
                printf("Trouvé: %s -> row=%d, col=%d\n", entry->d_name, pos.row, pos.col);
            }
        }
    }
    closedir(dir);
}

// Fonction principale
void creer_grille(const char *repertoire, const char *fichier_sortie) {
    Position positions[MAX_FILES];
    int nb_positions = 0;
    
    // Parcourir récursivement tous les sous-dossiers
    parcourir_dossiers(repertoire, positions, &nb_positions);
    
    printf("Nombre de fichiers trouvés: %d\n", nb_positions);
    
    if (nb_positions == 0) {
        printf("Aucune position trouvée.\n");
        return;
    }
    
    // Trier les positions
    qsort(positions, nb_positions, sizeof(Position), comparer_positions);
    
    // Trouver les dimensions de la grille et afficher les statistiques
    int max_row = 0, max_col = 0;
    int min_row = 999999, min_col = 999999;
    for (int i = 0; i < nb_positions; i++) {
        if (positions[i].row > max_row) max_row = positions[i].row;
        if (positions[i].col > max_col) max_col = positions[i].col;
        if (positions[i].row < min_row) min_row = positions[i].row;
        if (positions[i].col < min_col) min_col = positions[i].col;
    }
    
    printf("Statistiques:\n");
    printf("  Row: min=%d, max=%d\n", min_row, max_row);
    printf("  Col: min=%d, max=%d\n", min_col, max_col);
    
    int nb_rows = max_row - min_row + 1;
    int nb_cols = max_col - min_col + 1;
    
    printf("Dimensions de la grille: %d lignes x %d colonnes\n", nb_rows, nb_cols);
    printf("Positions attendues: %d (fichiers trouvés: %d)\n", nb_rows * nb_cols, nb_positions);
    
    // Créer la grille
    char **grille = malloc(nb_rows * sizeof(char *));
    for (int i = 0; i < nb_rows; i++) {
        grille[i] = malloc((nb_cols + 1) * sizeof(char));
        memset(grille[i], ' ', nb_cols);
        grille[i][nb_cols] = '\0';
    }
    
    // Placer les X aux positions (en normalisant avec min_row et min_col)
    for (int i = 0; i < nb_positions; i++) {
        int r = positions[i].row - min_row;
        int c = positions[i].col - min_col;
        grille[r][c] = 'X';
    }
    
    // Écrire dans le fichier
    FILE *f = fopen(fichier_sortie, "w");
    if (!f) {
        printf("Erreur: impossible de créer le fichier %s\n", fichier_sortie);
        return;
    }
    
    for (int i = 0; i < nb_rows; i++) {
        fprintf(f, "%s\n", grille[i]);
    }
    fclose(f);
    
    printf("Grille créée dans %s\n", fichier_sortie);
    
    // Libérer la mémoire
    for (int i = 0; i < nb_rows; i++) {
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