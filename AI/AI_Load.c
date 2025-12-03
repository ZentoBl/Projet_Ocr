
int load_network(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier de modele");
        return -1;
    }

    char buffer[256];
    
    if (fscanf(file, "%s\n", buffer) != 1 || strcmp(buffer, "---") != 0) goto error_read;
    for (int i = 0; i < INPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            if (fscanf(file, "%lf ", &weights_ih[i][j]) != 1) goto error_read;
        }
    }

    if (fscanf(file, "%s\n", buffer) != 1 || strcmp(buffer, "---") != 0) goto error_read; 
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        if (fscanf(file, "%lf ", &biases_h[j]) != 1) goto error_read;
    }

    if (fscanf(file, "%s\n", buffer) != 1 || strcmp(buffer, "---") != 0) goto error_read;
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        for (int k = 0; k < OUTPUT_SIZE; k++) {
            if (fscanf(file, "%lf ", &weights_ho[j][k]) != 1) goto error_read;
        }
    }

    if (fscanf(file, "%s\n", buffer) != 1 || strcmp(buffer, "---") != 0) goto error_read; 
    for (int k = 0; k < OUTPUT_SIZE; k++) {
        if (fscanf(file, "%lf ", &biases_o[k]) != 1) goto error_read;
    }

    fclose(file);
    return 0;

error_read:
    fprintf(stderr, "Erreur de format dans le fichier de modele. Verifiez la taille du reseau.\n");
    fclose(file);
    return -1;
}

char predict_image(const char *model_path, const char *image_path) {
    
    if (load_network(model_path) != 0) {
        fprintf(stderr, "Prediction annulee: Impossible de charger le modele depuis %s\n", model_path);
        return '?';
    }
    printf("Modele charge avec succes depuis %s.\n", model_path);


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Erreur SDL (initialisation prediction): %s\n", SDL_GetError());
        return '?';
    }

    SDL_Surface *image_surface = SDL_LoadBMP(image_path);
    if (!image_surface) {
        fprintf(stderr, "Erreur: Impossible de charger l'image BMP '%s': %s\n", image_path, SDL_GetError());
        SDL_Quit();
        return '?';
    }

    double *input_matrix = convert_surface_to_matrix(image_surface);
    
    SDL_FreeSurface(image_surface);
    SDL_Quit(); 

    if (input_matrix == NULL) {
        return '?';
    }

    forward_pass(input_matrix);
    char predicted_char = get_predicted_letter(output_outputs);
    
    double confidence = output_outputs[predicted_char - 'A'];
    
    printf("Image %s convertie et analysee.\n", image_path);
    printf("Confiance du reseau : %.4f\n", confidence);

    free(input_matrix);
    
    return predicted_char;
}