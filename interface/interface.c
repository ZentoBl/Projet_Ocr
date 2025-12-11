#include <gtk/gtk.h>
#include <math.h>

// Structure pour stocker les widgets et données de l'application
typedef struct {
    GtkBuilder *builder;
    GtkWidget *window;
    GtkWidget *image;
    GdkPixbuf *original_pixbuf;  // Pour stocker l'image originale
    double rotation_angle;        // Angle de rotation en degrés
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

// Gestionnaire pour le bouton de rotation automatique
void on_auto_rotate_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Vérifier qu'une image est chargée
    if (!app->original_pixbuf) {
        g_print("Aucune image à faire pivoter automatiquement\n");
        return;
    }
    
    // Sauvegarder l'image temporairement
    const char *temp_input = "automatic_rotation/temp_input.bmp";
    GError *error = NULL;
    
    // Récupérer le pixbuf actuel
    GdkPixbuf *current_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(app->image));
    
    if (current_pixbuf) {
        // Sauvegarder en BMP
        if (gdk_pixbuf_save(current_pixbuf, temp_input, "bmp", &error, NULL)) {
            g_print("Image sauvegardée pour rotation automatique: %s\n", temp_input);
            
            // Appeler le programme de rotation automatique avec ImageMagick
            gchar *command = g_strdup_printf("cd automatic_rotation && ./rotate_bmp_im temp_input.bmp");
            g_print("Exécution de: %s\n", command);
            
            int result = system(command);
            g_free(command);
            
            if (result == 0) {
                g_print("Rotation automatique terminée avec succès\n");
                
                // Charger l'image pivotée
                const char *rotated_filename = "automatic_rotation/final_rotated.bmp";
                GdkPixbuf *rotated_pixbuf = gdk_pixbuf_new_from_file(rotated_filename, &error);
                
                if (rotated_pixbuf) {
                    // Libérer l'ancien pixbuf original
                    if (app->original_pixbuf) {
                        g_object_unref(app->original_pixbuf);
                    }
                    
                    // Remplacer l'image originale par l'image pivotée
                    app->original_pixbuf = rotated_pixbuf;
                    
                    // Réinitialiser l'angle de rotation manuelle
                    app->rotation_angle = 0.0;
                    
                    // Spinbutton supprimé, pas de mise à jour nécessaire
                    
                    // Afficher l'image pivotée
                    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), app->original_pixbuf);
                    
                    g_print("Image pivotée automatiquement chargée et affichée\n");

                    // Sauvegarder l'image pivotée pour l'exécutable preprocess/get_letters
                    const char *saved_rotated_input = "../image_modifier/tests_images/gui_input_rotated.png";
                    GError *save_err = NULL;
                    if (gdk_pixbuf_save(app->original_pixbuf, saved_rotated_input, "png", &save_err, NULL)) {
                        g_print("Image pivotée sauvegardée: %s\n", saved_rotated_input);
                        // Lancer preprocess sur l'image pivotée pour produire le N&B
                        gchar *pre_cmd = g_strdup_printf("cd ../image_modifier && ./preprocess gui_input_rotated.png");
                        g_print("Exécution de: %s\n", pre_cmd);
                        int pre_res = system(pre_cmd);
                        g_free(pre_cmd);
                        if (pre_res == 0) {
                            g_print("Conversion Noir & Blanc (post-rotation) terminée avec succès\n");
                            // Appeler get_letters sur la sortie N&B
                            gchar *letters_cmd = g_strdup_printf("cd ../image_modifier && ./get_letters gui_input_rotated.png_bw.bmp");
                            g_print("Exécution de: %s\n", letters_cmd);
                            int letters_res = system(letters_cmd);
                            g_free(letters_cmd);
                            if (letters_res == 0) {
                                g_print("Détection des lettres terminée avec succès\n");
                            } else {
                                g_printerr("Erreur lors de l'exécution de get_letters (code: %d)\n", letters_res);
                            }
                        } else {
                            g_printerr("Erreur lors de l'exécution de preprocess (post-rotation) (code: %d)\n", pre_res);
                        }
                    } else {
                        g_printerr("Erreur lors de la sauvegarde de l'image pivotée: %s\n", save_err ? save_err->message : "inconnue");
                        if (save_err) g_error_free(save_err);
                    }
                } else {
                    g_printerr("Erreur lors du chargement de l'image pivotée: %s\n", 
                               error ? error->message : "fichier introuvable");
                    if (error) g_error_free(error);
                }
            } else {
                g_printerr("Erreur lors de l'exécution de la rotation automatique (code: %d)\n", result);
            }
        } else {
            g_printerr("Erreur lors de la sauvegarde temporaire: %s\n", error->message);
            g_error_free(error);
        }
    }
}

// Gestionnaire pour le bouton d'importation d'image
void on_import_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Créer le dialogue de sélection de fichier
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
        
        
        
        // Afficher l'image
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), app->original_pixbuf);
        
        g_print("Image chargée: %s\n", filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}// Gestionnaire pour le bouton de téléchargement
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



// Gestionnaire pour le bouton Noir & Blanc via executable preprocess
void on_convert_button_clicked(GtkButton *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Vérifier qu'une image est chargée
    if (!app->original_pixbuf) {
        g_print("Aucune image à convertir\n");
        return;
    }
    
    // Nom du fichier d'entrée pour preprocess (dans image_modifier/tests_images)
    const char *preprocess_input = "../image_modifier/tests_images/gui_input.png";
    
    // Récupérer le pixbuf actuel (avec rotation appliquée)
    GdkPixbuf *current_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(app->image));
    
    if (current_pixbuf) {
        GError *error = NULL;
        
        // Sauvegarder l'image actuelle en PNG pour preprocess
        if (gdk_pixbuf_save(current_pixbuf, preprocess_input, "png", &error, NULL)) {
            g_print("Image sauvegardée pour preprocess: %s\n", preprocess_input);

            // Appeler l'exécutable preprocess sur le fichier sauvegardé (il préfixe tests_images/ lui-même)
            gchar *command = g_strdup_printf("cd ../image_modifier/ && ./preprocess gui_input.png");
            g_print("Exécution de: %s\n", command);

            int result = system(command);
            g_free(command);

            if (result == 0) {
                g_print("Conversion Noir & Blanc terminée avec succès\n");

                // Chemin de sortie généré par preprocess
                const char *processed_filename = "../image_modifier/black_and_white/gui_input.png_bw.bmp";
                GdkPixbuf *processed_pixbuf = gdk_pixbuf_new_from_file(processed_filename, &error);

                if (processed_pixbuf) {
                    // Libérer l'ancien pixbuf original
                    if (app->original_pixbuf) {
                        g_object_unref(app->original_pixbuf);
                    }
                    
                    // Remplacer l'image originale par l'image traitée
                    app->original_pixbuf = processed_pixbuf;
                    
                    // Réinitialiser l'angle de rotation
                    app->rotation_angle = 0.0;
                    
                    // Afficher l'image traitée
                    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), app->original_pixbuf);
                    
                    g_print("Image Noir & Blanc chargée et affichée\n");
                } else {
                    g_printerr("Erreur lors du chargement de l'image Noir & Blanc: %s\n", 
                               error ? error->message : "fichier introuvable");
                    if (error) g_error_free(error);
                }
            } else {
                g_printerr("Erreur lors de l'exécution de preprocess (code: %d)\n", result);
            }
        } else {
            g_printerr("Erreur lors de la sauvegarde pour preprocess: %s\n", error->message);
            g_error_free(error);
        }
    }
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
    
    // Récupérer le bouton de conversion
    GtkWidget *convert_button = GTK_WIDGET(gtk_builder_get_object(app.builder, "convert_button"));
    
    if (!convert_button) {
        g_printerr("Impossible de trouver 'convert_button'\n");
        return 1;
    }

    // Récupérer le bouton de rotation automatique
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
    // Pas de connexion au spinbutton (supprimé)
    g_signal_connect(auto_rotate_button, "clicked", G_CALLBACK(on_auto_rotate_button_clicked), &app);
    g_signal_connect(convert_button, "clicked", G_CALLBACK(on_convert_button_clicked), &app);
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