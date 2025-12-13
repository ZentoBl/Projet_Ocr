#include <gtk/gtk.h>

// Structure to store the application widgets and data
typedef struct {
    GtkBuilder *builder;
    GtkWidget *window;
    GtkWidget *image;
    GdkPixbuf *original_pixbuf;  // To store the original image
    char input_filename[256];     // Input filename (without path)
} AppData;

// Handler for complete processing button
// (Black & White + Automatic Rotation)
void on_auto_rotate_button_clicked(GtkButton *button,
                                   gpointer user_data) {
    AppData *app = (AppData *)user_data;

    if (!app->original_pixbuf ||
        strlen(app->input_filename) == 0) {
        g_print("No image to process or missing "
                "filename\n");
        return;
    }

    GError *error = NULL;

    // Step 1: B&W pre-rotation
    g_print("=== Step 1: Black & White Conversion "
            "(pre-rotation) ===\n");
    gchar *temp_filename = g_strdup_printf(
        "gui_processed_%s", app->input_filename);
    gchar *preprocess_input = g_strdup_printf(
        "../image_modifier/tests_images/%s",
        temp_filename);
    GdkPixbuf *current_pixbuf = gtk_image_get_pixbuf(
        GTK_IMAGE(app->image));
    if (!current_pixbuf) {
        g_printerr("Error: unable to retrieve current "
                   "image\n");
        g_free(preprocess_input);
        g_free(temp_filename);
        return;
    }
    if (!gdk_pixbuf_save(current_pixbuf,
                         preprocess_input, "png",
                         &error, NULL)) {
        g_printerr("Error during save for preprocess: "
                   "%s\n", error->message);
        g_error_free(error);
        g_free(preprocess_input);
        g_free(temp_filename);
        return;
    }
    g_free(preprocess_input);
    gchar *preprocess_cmd = g_strdup_printf(
        "cd ../image_modifier/ && ./preprocess %s",
        temp_filename);
    int preprocess_res = system(preprocess_cmd);
    g_free(preprocess_cmd);
    if (preprocess_res != 0) {
        g_printerr("Preprocess error (code: %d)\n",
                   preprocess_res);
        g_free(temp_filename);
        return;
    }
    gchar *bw_filename = g_strdup_printf(
        "../image_modifier/black_and_white/%s_bw.bmp",
        temp_filename);
    GdkPixbuf *bw_pixbuf = gdk_pixbuf_new_from_file(
        bw_filename, &error);
    if (!bw_pixbuf) {
        g_printerr("Error loading B&W: %s\n",
                   error ? error->message :
                   "file not found");
        if (error) g_error_free(error);
        g_free(bw_filename);
        g_free(temp_filename);
        return;
    }
    g_free(bw_filename);

    // Step 2: Automatic rotation
    // (angle from B&W, rotation applied to original)
    g_print("=== Step 2: Automatic rotation ===\\n");
    // Save B&W as BMP for angle
    const char *nb_bmp = "automatic_rotation/"
                         "temp_input.bmp";
    if (!gdk_pixbuf_save(bw_pixbuf, nb_bmp, "bmp",
                         &error, NULL)) {
        g_printerr("Error saving BMP: %s\\n",
                   error->message);
        g_error_free(error);
        g_object_unref(bw_pixbuf);
        g_free(temp_filename);
        return;
    }
    // Path to original
    gchar *orig_path = g_strdup_printf(
        "../../image_modifier/tests_images/%s",
        app->input_filename);
    g_object_unref(bw_pixbuf);
    // Call rotate_bmp_im with 2 arguments:
    // B&W for angle, original for output
    gchar *rot_cmd = g_strdup_printf(
        "cd automatic_rotation && ./rotate_bmp_im "
        "temp_input.bmp \"%s\"", orig_path);
    int rot_res = system(rot_cmd);
    g_free(rot_cmd);
    g_free(orig_path);
    if (rot_res != 0) {
        g_printerr("Auto rotation error (code: %d)\n",
                   rot_res);
        g_free(temp_filename);
        return;
    }
    GdkPixbuf *rotated_pixbuf =
        gdk_pixbuf_new_from_file(
            "automatic_rotation/final_rotated.bmp",
            &error);
    if (!rotated_pixbuf) {
        g_printerr("Error loading rotated image: %s\n",
                   error ? error->message :
                   "file not found");
        if (error) g_error_free(error);
        g_free(temp_filename);
        return;
    }
    if (app->original_pixbuf)
        g_object_unref(app->original_pixbuf);
    app->original_pixbuf = rotated_pixbuf;
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image),
                              app->original_pixbuf);

    // Step 3: B&W post-rotation
    g_print("=== Step 3: Reprocess Black & White "
            "(post-rotation) ===\n");
    gchar *rotated_temp_filename = g_strdup_printf(
        "gui_rotated_%s", app->input_filename);
    gchar *rotated_temp_path = g_strdup_printf(
        "../image_modifier/tests_images/%s",
        rotated_temp_filename);
    if (!gdk_pixbuf_save(app->original_pixbuf,
                         rotated_temp_path, "png",
                         &error, NULL)) {
        g_printerr("Error saving rotated image: %s\n",
                   error->message);
        g_error_free(error);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gchar *reprocess_cmd = g_strdup_printf(
        "cd ../image_modifier && ./preprocess %s",
        rotated_temp_filename);
    int reprocess_res = system(reprocess_cmd);
    g_free(reprocess_cmd);
    if (reprocess_res != 0) {
        g_printerr("Error preprocess post-rotation "
                   "(code: %d)\n", reprocess_res);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gchar *reprocessed_bw_path = g_strdup_printf(
        "../image_modifier/black_and_white/"
        "%s_bw.bmp",
        rotated_temp_filename);
    GdkPixbuf *reprocessed_bw_pixbuf =
        gdk_pixbuf_new_from_file(reprocessed_bw_path,
                                 &error);
    if (!reprocessed_bw_pixbuf) {
        g_printerr("Error loading B&W post-rotation: "
                   "%s\n",
                   error ? error->message :
                   "file not found");
        if (error) g_error_free(error);
        g_free(reprocessed_bw_path);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image),
                              reprocessed_bw_pixbuf);

    // Step 4: get_letters
    g_print("=== Step 4: Letter detection ===\n");
    gchar *letters_input = g_strdup_printf(
        "%s_bw.bmp", rotated_temp_filename);
    gchar *letters_cmd = g_strdup_printf(
        "cd ../image_modifier && ./get_letters %s",
        letters_input);
    int letters_res = system(letters_cmd);
    g_free(letters_cmd);
    g_free(letters_input);
    if (letters_res != 0) {
        g_printerr("Get_letters error (code: %d)\n",
                   letters_res);
        g_object_unref(reprocessed_bw_pixbuf);
        g_free(reprocessed_bw_path);
        g_free(rotated_temp_path);
        g_free(rotated_temp_filename);
        g_free(temp_filename);
        return;
    }

    // Step 5: create_grid letters
    gchar *grid_cmd = g_strdup_printf(
        "cd ../Letter_txt_creator && "
        "./create_grid ../image_modifier/"
        "images_grid_letters grid.txt");
    int grid_res = system(grid_cmd);
    g_free(grid_cmd);
    if (grid_res != 0) {
        g_printerr("Error create_grid letters "
                   "(code: %d)\n", grid_res);
    }

    // Step 6: create_grid words
    gchar *word_cmd = g_strdup_printf(
        "cd ../Letter_txt_creator && "
        "./create_grid ../image_modifier/"
        "images_word_letters word.txt");
    int word_res = system(word_cmd);
    g_free(word_cmd);
    if (word_res != 0) {
        g_printerr("Error create_grid words "
                   "(code: %d)\n", word_res);
    }

    // Step 7: make run (final_result)
    if (word_res == 0) {
        g_print("=== Step 7: make run "
                "(final_result) ===\n");
        gchar *make_run_cmd =
            g_strdup("cd ../final_result && "
                     "make run");
        int make_run_res = system(make_run_cmd);
        g_free(make_run_cmd);
        if (make_run_res != 0) {
            g_printerr("Error make run "
                       "(code: %d)\n",
                       make_run_res);
        } else {
            // Load and display the annotated image
            // produced by final_result
            const char *annotated_path =
                "../final_result/annotated.png";
            error = NULL;
            GdkPixbuf *annotated_pixbuf =
                gdk_pixbuf_new_from_file(
                    annotated_path, &error);
            if (!annotated_pixbuf) {
                g_printerr("Error loading "
                           "annotated.png: %s\\n",
                           error ? error->message :
                           "file not found");
                if (error) g_error_free(error);
            } else {
                if (app->original_pixbuf)
                    g_object_unref(
                        app->original_pixbuf);
                app->original_pixbuf =
                    annotated_pixbuf;
                gtk_image_set_from_pixbuf(
                    GTK_IMAGE(app->image),
                    app->original_pixbuf);
                g_print("Annotated image "
                        "displayed: %s\\n",
                        annotated_path);
            }
        }
    }

    // Cleanup
    g_object_unref(reprocessed_bw_pixbuf);
    g_free(reprocessed_bw_path);
    g_free(rotated_temp_path);
    g_free(rotated_temp_filename);
    g_free(temp_filename);
}

// Handler for image import button
void on_import_button_clicked(GtkButton *button,
                              gpointer user_data) {
    AppData *app = (AppData *)user_data;

    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(
            "Select an image",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT,
            NULL
        );

    // Add a filter for images
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter,
                                  "image/png");
    gtk_file_filter_add_mime_type(filter,
                                  "image/jpeg");
    gtk_file_filter_add_mime_type(filter,
                                  "image/gif");
    gtk_file_filter_add_mime_type(filter,
                                  "image/bmp");
    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(dialog), filter);

    // Display the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) ==
        GTK_RESPONSE_ACCEPT) {
        char *filename =
            gtk_file_chooser_get_filename(
                GTK_FILE_CHOOSER(dialog));

        // Clean up old generated images
        // in subprojects
        g_print("Cleaning up generated images "
                "(cleanimg)...\n");
        int ci_res;
        gchar *ci_cmd;

        ci_cmd = g_strdup(
            "cd ../final_result && "
            "make cleanimg");
        ci_res = system(ci_cmd);
        g_free(ci_cmd);
        if (ci_res != 0) {
            g_printerr("Error cleanimg "
                       "(final_result): %d\n",
                       ci_res);
        }

        ci_cmd = g_strdup(
            "cd ../image_modifier && "
            "make cleanimg");
        ci_res = system(ci_cmd);
        g_free(ci_cmd);
        if (ci_res != 0) {
            g_printerr("Error cleanimg "
                       "(image_modifier): %d\n",
                       ci_res);
        }

        ci_cmd = g_strdup(
            "cd ../Letter_txt_creator && "
            "make cleanimg");
        ci_res = system(ci_cmd);
        g_free(ci_cmd);
        if (ci_res != 0) {
            g_printerr("Error cleanimg "
                       "(Letter_txt_creator): "
                       "%d\n", ci_res);
        }

        // Free old pixbuf if exists
        if (app->original_pixbuf) {
            g_object_unref(app->original_pixbuf);
        }

        // Load the new original pixbuf
        app->original_pixbuf =
            gdk_pixbuf_new_from_file(
                filename, NULL);

        // Extract filename without path
        const char *basename =
            g_path_get_basename(filename);
        g_strlcpy(app->input_filename, basename,
                  sizeof(app->input_filename));

        // Display the image
        gtk_image_set_from_pixbuf(
            GTK_IMAGE(app->image),
            app->original_pixbuf);

        g_print("Image loaded: %s\n", filename);
        g_print("Stored filename: %s\n",
                app->input_filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

// Handler for download button
void on_download_button_clicked(GtkButton *button,
                                gpointer user_data) {
    AppData *app = (AppData *)user_data;

    // Check that an image is loaded
    if (!app->original_pixbuf) {
        g_print("No image to download\n");
        return;
    }

    // Create the file save dialog
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(
            "Save image",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL
        );

    // Set default name
    gtk_file_chooser_set_current_name(
        GTK_FILE_CHOOSER(dialog), "image.bmp");

    // Add filters for different formats
    GtkFileFilter *filter_bmp =
        gtk_file_filter_new();
    gtk_file_filter_set_name(filter_bmp,
                             "BMP (*.bmp)");
    gtk_file_filter_add_pattern(filter_bmp,
                                "*.bmp");
    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(dialog), filter_bmp);

    GtkFileFilter *filter_png =
        gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png,
                             "PNG (*.png)");
    gtk_file_filter_add_pattern(filter_png,
                                "*.png");
    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(dialog), filter_png);

    GtkFileFilter *filter_jpg =
        gtk_file_filter_new();
    gtk_file_filter_set_name(filter_jpg,
                             "JPEG (*.jpg, "
                             "*.jpeg)");
    gtk_file_filter_add_pattern(filter_jpg,
                                "*.jpg");
    gtk_file_filter_add_pattern(filter_jpg,
                                "*.jpeg");
    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(dialog), filter_jpg);

    GtkFileFilter *filter_all =
        gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all,
                             "All files");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(
        GTK_FILE_CHOOSER(dialog), filter_all);

    // Display the dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) ==
        GTK_RESPONSE_ACCEPT) {
        char *filename =
            gtk_file_chooser_get_filename(
                GTK_FILE_CHOOSER(dialog));

        // Get the currently displayed image
        GdkPixbuf *current_pixbuf =
            gtk_image_get_pixbuf(
                GTK_IMAGE(app->image));

        if (current_pixbuf) {
            GError *error = NULL;

            // Determine format from extension
            const char *format = "bmp";
            if (g_str_has_suffix(filename,
                                 ".png")) {
                format = "png";
            } else if (g_str_has_suffix(
                           filename, ".jpg") ||
                       g_str_has_suffix(
                           filename, ".jpeg")) {
                format = "jpeg";
            }

            // Save the image
            if (gdk_pixbuf_save(current_pixbuf,
                                filename, format,
                                &error, NULL)) {
                g_print("Image saved: %s\n",
                        filename);
            } else {
                g_printerr("Error saving: %s\n",
                           error->message);
                g_error_free(error);
            }
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

// Handler for window close
void on_window_destroy(GtkWidget *widget,
                       gpointer user_data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    AppData app;
    GError *error = NULL;

    // Initialize variables
    app.original_pixbuf = NULL;
    memset(app.input_filename, 0,
           sizeof(app.input_filename));

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the builder and load the Glade file
    app.builder = gtk_builder_new();

    if (!gtk_builder_add_from_file(app.builder,
                                   "interface.glade",
                                   &error)) {
        g_printerr("Error loading file: %s\n",
                   error->message);
        g_error_free(error);
        return 1;
    }

    // Get the main window
    app.window =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "main_window"));

    if (!app.window) {
        g_printerr("Cannot find 'main_window'\n");
        return 1;
    }

    // Get the image widget
    app.image =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "image_display"));

    if (!app.image) {
        g_printerr("Cannot find "
                   "'image_display'\n");
        return 1;
    }

    // Get the import button
    GtkWidget *import_button =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "import_button"));

    if (!import_button) {
        g_printerr("Cannot find "
                   "'import_button'\n");
        return 1;
    }

    // The rotation spinbutton has been
    // removed from the interface

    // Get the automatic processing button
    // (B&W + Rotation)
    GtkWidget *auto_rotate_button =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "auto_rotate_button"));

    if (!auto_rotate_button) {
        g_printerr("Cannot find "
                   "'auto_rotate_button'\n");
        return 1;
    }

    // Get the download button
    GtkWidget *download_button =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "download_button"));

    if (!download_button) {
        g_printerr("Cannot find "
                   "'download_button'\n");
        return 1;
    }

    // Get the header bar buttons
    GtkWidget *header_import_button =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "header_import_button"));
    GtkWidget *header_save_button =
        GTK_WIDGET(gtk_builder_get_object(
            app.builder, "header_save_button"));

    // Connect signals manually
    g_signal_connect(app.window, "destroy",
                     G_CALLBACK(on_window_destroy),
                     NULL);
    g_signal_connect(import_button, "clicked",
                     G_CALLBACK(
                         on_import_button_clicked),
                     &app);
    g_signal_connect(auto_rotate_button,
                     "clicked",
                     G_CALLBACK(
                         on_auto_rotate_button_clicked),
                     &app);
    g_signal_connect(download_button, "clicked",
                     G_CALLBACK(
                         on_download_button_clicked),
                     &app);

    // Connect header buttons to the same
    // functions
    if (header_import_button) {
        g_signal_connect(header_import_button,
                         "clicked",
                         G_CALLBACK(
                             on_import_button_clicked),
                         &app);
    }
    if (header_save_button) {
        g_signal_connect(header_save_button,
                         "clicked",
                         G_CALLBACK(
                             on_download_button_clicked),
                         &app);
    }

    // Display the window
    gtk_widget_show_all(app.window);

    // Start the main loop
    gtk_main();

    return 0;
}
