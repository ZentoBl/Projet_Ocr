#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <MagickWand/MagickWand.h>

// Paramètres de recherche
#define MIN_ANGLE -45.0
#define MAX_ANGLE  45.0
#define STEP_ANGLE  1.0   // Précision de 1 degré (suffisant pour lire)
#define WORK_WIDTH  600   // Redimensionner l'analyse pour la vitesse

static void ThrowWandException(MagickWand *wand) {
    char *description;
    ExceptionType severity;
    description = MagickGetException(wand, &severity);
    fprintf(stderr, "Erreur : %s\n", description);
    description = (char *) MagickRelinquishMemory(description);
    exit(1);
}

// Fonction pour calculer la variance des projections horizontales
// Plus le score est haut, plus l'image est "droite"
double calculate_alignment_score(MagickWand *wand) {
    size_t width = MagickGetImageWidth(wand);
    size_t height = MagickGetImageHeight(wand);
    
    // On récupère les pixels en brut (Gris 'I' = Intensité)
    unsigned char *pixels = malloc(width * height * sizeof(unsigned char));
    if (!pixels) return 0.0;
    
    MagickExportImagePixels(wand, 0, 0, width, height, "I", CharPixel, pixels);

    unsigned long *row_sums = calloc(height, sizeof(unsigned long));
    double total_sum = 0.0;

    // On somme les valeurs de pixels ligne par ligne
    // (Dans votre image : Noir = 0, Blanc = 255)
    // Pour maximiser le contraste, on inverse (on compte le noir)
    for (size_t y = 0; y < height; y++) {
        unsigned long current_row_sum = 0;
        size_t offset = y * width;
        for (size_t x = 0; x < width; x++) {
            // Si le pixel est foncé (< 128), on l'ajoute
            if (pixels[offset + x] < 128) { 
                current_row_sum++; 
            }
        }
        row_sums[y] = current_row_sum;
        total_sum += current_row_sum;
    }

    double mean = total_sum / (double)height;
    double variance = 0.0;

    for (size_t y = 0; y < height; y++) {
        double diff = (double)row_sums[y] - mean;
        variance += diff * diff;
    }

    free(row_sums);
    free(pixels);
    return variance / height;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image.bmp>\n", argv[0]);
        return 1;
    }

    MagickWandGenesis();
    MagickWand *orig = NewMagickWand();
    MagickWand *worker = NewMagickWand();
    PixelWand *white_bg = NewPixelWand();

    PixelSetColor(white_bg, "white");

    // 1. Lecture
    if (MagickReadImage(orig, argv[1]) == MagickFalse) 
        ThrowWandException(orig);

    // 2. Préparation de l'image de travail (copie réduite pour la vitesse)
    worker = CloneMagickWand(orig);
    
    // On redimensionne si l'image est trop grosse (> WORK_WIDTH) pour accélérer le calcul
    size_t w = MagickGetImageWidth(orig);
    size_t h = MagickGetImageHeight(orig);
    if (w > WORK_WIDTH) {
        double ratio = (double)h / w;
        MagickResizeImage(worker, WORK_WIDTH, (size_t)(WORK_WIDTH * ratio), LanczosFilter);
    }

    // On s'assure que c'est bien binaire pour l'analyse
    MagickSetImageColorspace(worker, GRAYColorspace);
    MagickThresholdImage(worker, 0.5 * QuantumRange);

    printf("Recherche de l'angle optimal (%dpx de large)...\n", (int)MagickGetImageWidth(worker));

    double best_angle = 0.0;
    double max_score = -1.0;

    // 3. Boucle de test brute force intelligente
    for (double angle = MIN_ANGLE; angle <= MAX_ANGLE; angle += STEP_ANGLE) {
        MagickWand *temp = CloneMagickWand(worker);
        
        // Rotation avec fond blanc
        MagickRotateImage(temp, white_bg, angle);
        
        // Calcul du score
        double score = calculate_alignment_score(temp);
        
        if (score > max_score) {
            max_score = score;
            best_angle = angle;
        }
        
        DestroyMagickWand(temp);
    }

    printf("Angle détecté : %.1f degrés (Score: %.0f)\n", best_angle, max_score);

    // 4. Application finale sur l'image ORIGINALE (Haute Qualité)
    // On applique la rotation inverse ou directe selon le besoin.
    // MagickRotateImage tourne dans le sens horaire pour un angle positif.
    MagickRotateImage(orig, white_bg, best_angle);

    char outname[256];
    snprintf(outname, 256, "final_rotated.bmp");
    MagickWriteImage(orig, outname);
    printf("Image sauvegardée : %s\n", outname);

    DestroyPixelWand(white_bg);
    DestroyMagickWand(worker);
    DestroyMagickWand(orig);
    MagickWandTerminus();
    return 0;
}