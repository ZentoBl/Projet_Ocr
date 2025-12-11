#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <MagickWand/MagickWand.h>

// Paramètres de recherche
#define MIN_ANGLE    -45.0
#define MAX_ANGLE     45.0
#define COARSE_STEP    3.0   // balayage grossier
#define FINE_WINDOW    3.0   // fenêtre autour du meilleur angle grossier
#define FINE_STEP      1  // balayage fin
#define WORK_WIDTH   600     // Redimensionner l'analyse pour la vitesse

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
    unsigned char *pixels = (unsigned char*) malloc(width * height * sizeof(unsigned char));
    if (!pixels) return 0.0;
    
    MagickExportImagePixels(wand, 0, 0, width, height, "I", CharPixel, pixels);

    unsigned long *row_sums = (unsigned long*) calloc(height, sizeof(unsigned long));
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

    // Paramètre virtual pixel pour accélérer la rotation
    MagickSetImageVirtualPixelMethod(worker, BackgroundVirtualPixelMethod);

    // Dimensions de référence (pour recadrer après rotation et éviter d'exporter de grosses images)
    const size_t base_w = MagickGetImageWidth(worker);
    const size_t base_h = MagickGetImageHeight(worker);

    printf("Recherche de l'angle optimal (%dpx de large)...\n", (int)MagickGetImageWidth(worker));

    double best_angle = 0.0;
    double max_score = -1.0;

    // 3. Recherche en deux passes (grossière puis fine)
    // 3.a Passer grossière
    for (double angle = MIN_ANGLE; angle <= MAX_ANGLE; angle += COARSE_STEP) {
        MagickWand *temp = CloneMagickWand(worker);
        MagickRotateImage(temp, white_bg, angle);
        // Recadrer au centre aux dimensions de référence pour un score plus stable et export plus petit
        size_t rw = MagickGetImageWidth(temp);
        size_t rh = MagickGetImageHeight(temp);
        ssize_t cx = (ssize_t)((rw > base_w) ? (rw - base_w) / 2 : 0);
        ssize_t cy = (ssize_t)((rh > base_h) ? (rh - base_h) / 2 : 0);
        if (rw != base_w || rh != base_h) {
            MagickCropImage(temp, base_w, base_h, cx, cy);
        }
        double score = calculate_alignment_score(temp);
        if (score > max_score) {
            max_score = score;
            best_angle = angle;
        }
        DestroyMagickWand(temp);
    }

    // 3.b Affinage autour du meilleur angle grossier
    double refine_min = best_angle - FINE_WINDOW;
    double refine_max = best_angle + FINE_WINDOW;
    if (refine_min < MIN_ANGLE) refine_min = MIN_ANGLE;
    if (refine_max > MAX_ANGLE) refine_max = MAX_ANGLE;

    for (double angle = refine_min; angle <= refine_max; angle += FINE_STEP) {
        MagickWand *temp = CloneMagickWand(worker);
        MagickRotateImage(temp, white_bg, angle);
        size_t rw = MagickGetImageWidth(temp);
        size_t rh = MagickGetImageHeight(temp);
        ssize_t cx = (ssize_t)((rw > base_w) ? (rw - base_w) / 2 : 0);
        ssize_t cy = (ssize_t)((rh > base_h) ? (rh - base_h) / 2 : 0);
        if (rw != base_w || rh != base_h) {
            MagickCropImage(temp, base_w, base_h, cx, cy);
        }
        double score = calculate_alignment_score(temp);
        if (score > max_score) {
            max_score = score;
            best_angle = angle;
        }
        DestroyMagickWand(temp);
    }

    printf("Angle détecté : %.1f degrés (Score: %.0f)\n", best_angle, max_score);

    // 4. Choisir le signe d'angle qui redresse le mieux
    double apply_angle = best_angle;
    {
        // Évaluer score pour best_angle
        MagickWand *testA = CloneMagickWand(worker);
        MagickRotateImage(testA, white_bg, best_angle);
        size_t rwA = MagickGetImageWidth(testA);
        size_t rhA = MagickGetImageHeight(testA);
        ssize_t cxA = (ssize_t)((rwA > base_w) ? (rwA - base_w) / 2 : 0);
        ssize_t cyA = (ssize_t)((rhA > base_h) ? (rhA - base_h) / 2 : 0);
        if (rwA != base_w || rhA != base_h) {
            MagickCropImage(testA, base_w, base_h, cxA, cyA);
        }
        double scoreA = calculate_alignment_score(testA);
        DestroyMagickWand(testA);

        // Évaluer score pour -best_angle
        MagickWand *testB = CloneMagickWand(worker);
        MagickRotateImage(testB, white_bg, -best_angle);
        size_t rwB = MagickGetImageWidth(testB);
        size_t rhB = MagickGetImageHeight(testB);
        ssize_t cxB = (ssize_t)((rwB > base_w) ? (rwB - base_w) / 2 : 0);
        ssize_t cyB = (ssize_t)((rhB > base_h) ? (rhB - base_h) / 2 : 0);
        if (rwB != base_w || rhB != base_h) {
            MagickCropImage(testB, base_w, base_h, cxB, cyB);
        }
        double scoreB = calculate_alignment_score(testB);
        DestroyMagickWand(testB);

        // Choisir l'angle avec le meilleur score
        if (scoreB > scoreA) {
            apply_angle = -best_angle;
        } else {
            apply_angle = best_angle;
        }
        printf("Application angle: %.2f° (scoreA=%.0f, scoreB=%.0f)\n", apply_angle, scoreA, scoreB);
    }

    // 5. Application finale sur l'image ORIGINALE (Haute Qualité)
    // MagickRotateImage tourne dans le sens horaire pour un angle positif.
    MagickRotateImage(orig, white_bg, apply_angle);

    // 6. Recadrage léger : enlever seulement les bandes blanches extrêmes
    // Réduire légèrement si l'image a grandi (limiter à +20% max)
    size_t rotated_w = MagickGetImageWidth(orig);
    size_t rotated_h = MagickGetImageHeight(orig);
    if (rotated_w > 1300 || rotated_h > 1100) {  // Seuils adapté au doc standard
        // Recadrer modérément
        size_t new_w = (rotated_w > 1300) ? 1300 : rotated_w;
        size_t new_h = (rotated_h > 1100) ? 1100 : rotated_h;
        ssize_t offset_x = (ssize_t)((rotated_w - new_w) / 2);
        ssize_t offset_y = (ssize_t)((rotated_h - new_h) / 2);
        MagickCropImage(orig, new_w, new_h, offset_x, offset_y);
    }

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