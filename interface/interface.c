#include <gtk/gtk.h>
#include <math.h>

// Structure pour stocker les widgets et données de l'application
typedef struct {
    GtkBuilder *builder;
    GtkWidget *window;
    GtkWidget *image;
    GdkPixbuf *original_pixbuf;  // Pour stocker l'image originale
    double rotation_angle;        // Angle de rotation en degrés
    char input_filename[256];     // Nom du fichier d'entrée (sans chemin)
        // char input_filepath[512];     // Chemin complet du fichier d'entrée
} AppData;

// Fonction pour appliquer la rotation à l'image avec Cairo
void apply_rotation(AppData *app) {
    if (!app->original_pixbuf) {
        return;
    }
    
    // Convertir l'angle en radians
    double angle_rad = app->rotation_angle * G_PI / 180.0;
    
    // Obtenir les dimensions de l'image originale
    int orig_width = gdk_pixbuf_get_width(app->original_pixbuf);
    int orig_height = gdk_pixbuf_get_height(app->original_pixbuf);
    
    // Calculer les nouvelles dimensions après rotation
    double cos_a = fabs(cos(angle_rad));
    double sin_a = fabs(sin(angle_rad));
    int new_width = (int)(orig_width * cos_a + orig_height * sin_a);
    int new_height = (int)(orig_width * sin_a + orig_height * cos_a);
    
    // Créer une surface Cairo
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
    cairo_t *cr = cairo_create(surface);
    
    // Remplir le fond en transparent
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    
    // Translater au centre de la nouvelle surface
    cairo_translate(cr, new_width / 2.0, new_height / 2.0);
    
    // Appliquer la rotation
    cairo_rotate(cr, angle_rad);
    
    // Translater pour centrer l'image originale
    cairo_translate(cr, -orig_width / 2.0, -orig_height / 2.0);
    
    // Dessiner l'image
    gdk_cairo_set_source_pixbuf(cr, app->original_pixbuf, 0, 0);
    cairo_paint(cr);
    
    // Créer un nouveau pixbuf à partir de la surface
    GdkPixbuf *rotated = gdk_pixbuf_get_from_surface(surface, 0, 0, new_width, new_height);
    
    // Afficher l'image tournée
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), rotated);
    
    // Nettoyer
    g_object_unref(rotated);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

// Gestionnaire pour le changement de valeur du spinbutton
void on_rotation_value_changed(GtkSpinButton *spin, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Récupérer la nouvelle valeur
    app->rotation_angle = gtk_spin_button_get_value(spin);
    
    g_print("Rotation: %.1f degrés\n", app->rotation_angle);
    
    // Appliquer la rotation
    apply_rotation(app);
}

// Gestionnaire pour le bouton de traitement complet (Noir & Blanc + Rotation automatique)
void on_auto_rotate_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    if (!app->original_pixbuf || strlen(app->input_filename) == 0) {
        g_print("Aucune image à traiter ou nom manquant\n");
        return;
    }

    GError *error = NULL;

    // Étape 1: N&B pré-rotation
    g_print("=== Étape 1: Conversion Noir & Blanc (pré-rotation) ===\n");
    gchar *temp_filename = g_strdup_printf("gui_processed_%s", app->input_filename);
    gchar *preprocess_input = g_strdup_printf("../image_modifier/tests_images/%s", temp_filename);
    GdkPixbuf *current_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(app->image));
    if (!current_pixbuf) {
        g_printerr("Erreur: impossible de récupérer l'image actuelle\n");
        g_free(preprocess_input);
        g_free(temp_filename);
        return;
    }
    if (!gdk_pixbuf_save(current_pixbuf, preprocess_input, "png", &error, NULL)) {
        g_printerr("Erreur lors de la sauvegarde pour preprocess: %s\n", error->message);
        g_error_free(error);
        g_free(preprocess_input);
        g_free(temp_filename);
        return;
    }
    g_free(preprocess_input);
    gchar *preprocess_cmd = g_strdup_printf("cd ../image_modifier/ && ./preprocess %s", temp_filename);
    int preprocess_res = system(preprocess_cmd);
    g_free(preprocess_cmd);
    if (preprocess_res != 0) {
        g_printerr("Erreur preprocess (code: %d)\n", preprocess_res);
        g_free(temp_filename);
        return;
    }
    gchar *bw_filename = g_strdup_printf("../image_modifier/black_and_white/%s_bw.bmp", temp_filename);
    GdkPixbuf *bw_pixbuf = gdk_pixbuf_new_from_file(bw_filename, &error);
    if (!bw_pixbuf) {
        g_printerr("Erreur chargement N&B: %s\n", error ? error->message : "fichier introuvable");
        if (error) g_error_free(error);
        g_free(bw_filename);
        g_free(temp_filename);
        return;
    }
    g_free(bw_filename);

    // Étape 2: Rotation automatique (angle depuis N&B, rotation appliquée à l'original)
    g_print("=== Étape 2: Rotation automatique ===\\n");
    // Sauvegarder la N&B en BMP pour l'angle
    const char *nb_bmp = "automatic_rotation/temp_input.bmp";
    if (!gdk_pixbuf_save(bw_pixbuf, nb_bmp, "bmp", &error, NULL)) {
        g_printerr("Erreur sauvegarde BMP: %s\\n", error->message);
        g_error_free(error);
        g_object_unref(bw_pixbuf);
        g_free(temp_filename);
        return;
    }
    // Chemin de l'original
    gchar *orig_path = g_strdup_printf("../../image_modifier/tests_images/%s", app->input_filename);
    g_object_unref(bw_pixbuf);
    // Appel à rotate_bmp_im avec 2 arguments: NB pour angle, original pour sortie
    gchar *rot_cmd = g_strdup_printf("cd automatic_rotation && ./rotate_bmp_im temp_input.bmp \"%s\"", orig_path);
    int rot_res = system(rot_cmd);
    g_free(rot_cmd);
    g_free(orig_path);
    if (rot_res != 0) {
        g_printerr("Erreur rotation auto (code: %d)\n", rot_res);
        g_free(temp_filename);
        return;
    }
    GdkPixbuf *rotated_pixbuf = gdk_pixbuf_new_from_file("automatic_rotation/final_rotated.bmp", &error);
    if (!rotated_pixbuf) {
        g_printerr("Erreur chargement image pivotée: %s\n", error ? error->message : "fichier introuvable");
        if (error) g_error_free(error);
        g_free(temp_filename);
        return;
    }
    if (app->original_pixbuf) g_object_unref(app->original_pixbuf);
    app->original_pixbuf = rotated_pixbuf;
    app->rotation_angle = 0.0;
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), app->original_pixbuf);

    // Étape 3: N&B post-rotation
    g_print("=== Étape 3: Reprocess Noir & Blanc (post-rotation) ===\n");
    gchar *rotated_temp_filename = g_strdup_printf("gui_rotated_%s", app->input_filename);
    gchar *rotated_temp_path = g_strdup_printf("../image_modifier/tests_images/%s", rotated_temp_filename);
    if (!gdk_pixbuf_save(app->original_pixbuf, rotated_temp_path, "png", &error, NULL)) {
        g_printerr("Erreur sauvegarde pivotée: %s\n", error->message);
        g_error_free(error);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gchar *reprocess_cmd = g_strdup_printf("cd ../image_modifier && ./preprocess %s", rotated_temp_filename);
    int reprocess_res = system(reprocess_cmd);
    g_free(reprocess_cmd);
    if (reprocess_res != 0) {
        g_printerr("Erreur preprocess post-rotation (code: %d)\n", reprocess_res);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gchar *reprocessed_bw_path = g_strdup_printf("../image_modifier/black_and_white/%s_bw.bmp", rotated_temp_filename);
    GdkPixbuf *reprocessed_bw_pixbuf = gdk_pixbuf_new_from_file(reprocessed_bw_path, &error);
    if (!reprocessed_bw_pixbuf) {
        g_printerr("Erreur chargement N&B post-rotation: %s\n", error ? error->message : "fichier introuvable");
        if (error) g_error_free(error);
        g_free(reprocessed_bw_path);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), reprocessed_bw_pixbuf);

    // Étape 4: get_letters
    g_print("=== Étape 4: Détection des lettres ===\n");
    gchar *letters_input = g_strdup_printf("%s_bw.bmp", rotated_temp_filename);
    gchar *letters_cmd = g_strdup_printf("cd ../image_modifier && ./get_letters %s", letters_input);
    int letters_res = system(letters_cmd);
    g_free(letters_cmd);
    g_free(letters_input);
    if (letters_res != 0) {
        g_printerr("Erreur get_letters (code: %d)\n", letters_res);
        g_object_unref(reprocessed_bw_pixbuf);
        g_free(reprocessed_bw_path);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }

    // Étape 5: create_grid lettres
    gchar *grid_cmd = g_strdup_printf("cd ../Letter_txt_creator && ./create_grid ../image_modifier/images_grid_letters grid.txt");
    int grid_res = system(grid_cmd);
    g_free(grid_cmd);
    if (grid_res != 0) {
        g_printerr("Erreur create_grid lettres (code: %d)\n", grid_res);
    }

    // Étape 6: create_grid mots
    gchar *word_cmd = g_strdup_printf("cd ../Letter_txt_creator && ./create_grid ../image_modifier/images_word_letters word.txt");
    int word_res = system(word_cmd);
    g_free(word_cmd);
    if (word_res != 0) {
        g_printerr("Erreur create_grid mots (code: %d)\n", word_res);
    }

    // Nettoyage
    g_object_unref(reprocessed_bw_pixbuf);
    g_free(reprocessed_bw_path);
    g_free(rotated_temp_path);
    g_free(rotated_temp_filename);
    g_free(temp_filename);
}

// Gestionnaire pour le bouton d'importation d'image
void on_import_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Sélectionner une image",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Ouvrir", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Ajouter un filtre pour les images
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/gif");
    gtk_file_filter_add_mime_type(filter, "image/bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Afficher le dialogue
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Libérer l'ancien pixbuf si existant
        if (app->original_pixbuf) {
            g_object_unref(app->original_pixbuf);
        }
        
        // Charger le nouveau pixbuf original
        app->original_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
        
        // Réinitialiser l'angle de rotation
        app->rotation_angle = 0.0;
        
        // Extraire le nom du fichier sans le chemin
        const char *basename = g_path_get_basename(filename);
        g_strlcpy(app->input_filename, basename, sizeof(app->input_filename));
        
        // Afficher l'image
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), app->original_pixbuf);
        
        g_print("Image chargée: %s\n", filename);
        g_print("Nom de fichier stocké: %s\n", app->input_filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

// Gestionnaire pour le bouton de téléchargement
void on_download_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Vérifier qu'une image est chargée
    if (!app->original_pixbuf) {
        g_print("Aucune image à télécharger\n");
        return;
    }
    
    // Créer le dialogue de sauvegarde de fichier
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Enregistrer l'image",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Annuler", GTK_RESPONSE_CANCEL,
        "_Enregistrer", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Définir le nom par défaut
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "image.bmp");
    
    // Ajouter des filtres pour différents formats
    GtkFileFilter *filter_bmp = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_bmp, "BMP (*.bmp)");
    gtk_file_filter_add_pattern(filter_bmp, "*.bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_bmp);
    
    GtkFileFilter *filter_png = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png, "PNG (*.png)");
    gtk_file_filter_add_pattern(filter_png, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_png);
    
    GtkFileFilter *filter_jpg = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_jpg, "JPEG (*.jpg, *.jpeg)");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpg");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_jpg);
    
    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "Tous les fichiers");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);
    
    // Afficher le dialogue
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // Récupérer l'image actuellement affichée (avec rotation si appliquée)
        GdkPixbuf *current_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(app->image));
        
        if (current_pixbuf) {
            GError *error = NULL;
            
            // Déterminer le format à partir de l'extension
            const char *format = "bmp";  // Format par défaut
            if (g_str_has_suffix(filename, ".png")) {
                format = "png";
            } else if (g_str_has_suffix(filename, ".jpg") || g_str_has_suffix(filename, ".jpeg")) {
                format = "jpeg";
            }
            
            // Sauvegarder l'image
            if (gdk_pixbuf_save(current_pixbuf, filename, format, &error, NULL)) {
                g_print("Image téléchargée: %s\n", filename);
            } else {
                g_printerr("Erreur lors de la sauvegarde: %s\n", error->message);
                g_error_free(error);
            }
        }
        
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}



// Gestionnaire pour la fermeture de la fenêtre
void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    AppData app;
    GError *error = NULL;
    
    // Initialiser les variables
    app.original_pixbuf = NULL;
    app.rotation_angle = 0.0;
    memset(app.input_filename, 0, sizeof(app.input_filename));
        // memset(app.input_filepath, 0, sizeof(app.input_filepath));
    
    // Initialiser GTK
    gtk_init(&argc, &argv);
    
    // Créer le builder et charger le fichier Glade
    app.builder = gtk_builder_new();
    
    if (!gtk_builder_add_from_file(app.builder, "interface.glade", &error)) {
        g_printerr("Erreur de chargement du fichier: %s\n", error->message);
        g_error_free(error);
        return 1;
    }
    
    // Récupérer la fenêtre principale
    app.window = GTK_WIDGET(gtk_builder_get_object(app.builder, "main_window"));
    
    if (!app.window) {
        g_printerr("Impossible de trouver 'main_window'\n");
        return 1;
    }
    
    // Récupérer le widget image
    app.image = GTK_WIDGET(gtk_builder_get_object(app.builder, "image_display"));
    
    if (!app.image) {
        g_printerr("Impossible de trouver 'image_display'\n");
        return 1;
    }
    
    // Récupérer le bouton d'import
    GtkWidget *import_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "import_button"));
    
    if (!import_button) {
        g_printerr("Impossible de trouver 'import_button'\n");
        return 1;
    }
    
    // Le spinbutton de rotation a été retiré de l'interface

    // Récupérer le bouton de traitement automatique (N&B + Rotation)
    GtkWidget *auto_rotate_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "auto_rotate_button"));
    
    if (!auto_rotate_button) {
        g_printerr("Impossible de trouver 'auto_rotate_button'\n");
        return 1;
    }

    // Récupérer le bouton de téléchargement
    GtkWidget *download_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "download_button"));
    
    if (!download_button) {
        g_printerr("Impossible de trouver 'download_button'\n");
        return 1;
    }
    
    // Récupérer les boutons du header bar
    GtkWidget *header_import_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "header_import_button"));
    GtkWidget *header_save_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "header_save_button"));
    
    // Connecter les signaux manuellement
    g_signal_connect(app.window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(import_button, "clicked", G_CALLBACK(on_import_button_clicked), &app);
    g_signal_connect(auto_rotate_button, "clicked", G_CALLBACK(on_auto_rotate_button_clicked), &app);
    g_signal_connect(download_button, "clicked", G_CALLBACK(on_download_button_clicked), &app);
    
    // Connecter les boutons du header aux mêmes fonctions
    if (header_import_button) {
        g_signal_connect(header_import_button, "clicked", G_CALLBACK(on_import_button_clicked), &app);
    }
    if (header_save_button) {
        g_signal_connect(header_save_button, "clicked", G_CALLBACK(on_download_button_clicked), &app);
    }
    
    // Afficher la fenêtre
    gtk_widget_show_all(app.window);
    
    // Lancer la boucle principale
    gtk_main();
    
    return 0;
}