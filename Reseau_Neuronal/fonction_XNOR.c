#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Fonction d'activation sigmoid
double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

// Dérivée de sigmoid (nécessaire pour backpropagation)
double sigmoid_derivative(double x)
{
    double sig = sigmoid(x);
    return sig * (1.0 - sig);
}

// Structure du réseau neuronal pour !A.!B + A.B
typedef struct 
{
    // Couche cachée : 2 neurones pour calculer !A.!B et A.B
    double hidden_weights[2][2];  // [neurone][entrée]
    double hidden_bias[2];
    
    // Couche de sortie : combine les deux résultats
    double output_weights[2];
    double output_bias;
    
    // Taux d'apprentissage
    double learning_rate;
} NeuralNetwork;

// Initialisation ALEATOIRE du réseau (pour l'apprentissage)
void init_network_random(NeuralNetwork *nn)
{
    srand(time(NULL));
    
    // Initialisation aléatoire entre -1 et 1
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            nn->hidden_weights[i][j] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        }
        nn->hidden_bias[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        nn->output_weights[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    }
    nn->output_bias = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    
    nn->learning_rate = 0.5;  // Taux d'apprentissage
}



// Propagation avant (forward pass)
void forward(NeuralNetwork *nn, double a, double b, double *hidden, 
    double *hidden_sum, double *output, double *output_sum)
{
    // Couche cachée : calcul des deux neurones
    for (int i = 0; i < 2; i++)
    {
        hidden_sum[i] = nn->hidden_weights[i][0] * a + 
                        nn->hidden_weights[i][1] * b + 
                        nn->hidden_bias[i];
        hidden[i] = sigmoid(hidden_sum[i]);
    }
    
    // Couche de sortie
    *output_sum = nn->output_weights[0] * hidden[0] + 
                  nn->output_weights[1] * hidden[1] + 
                  nn->output_bias;
    *output = sigmoid(*output_sum);
}

// Prédiction (0 ou 1)
int predict(NeuralNetwork *nn, double a, double b) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    return (output > 0.5) ? 1 : 0;
}

// Calcul de l'erreur (loss)
double compute_loss(NeuralNetwork *nn, double a, double b, 
    double expected) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    double error = output - expected;
    return error * error;  // Erreur quadratique (MSE)
}

// BACKPROPAGATION - Le cœur de l'apprentissage !
void backpropagate(NeuralNetwork *nn, double a, double b, 
    double expected)
{
    double hidden[2], hidden_sum[2], output, output_sum;
    
    // 1. Forward pass pour obtenir toutes les valeurs
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    
    // 2. Calcul de l'erreur en sortie
    double output_error = output - expected;
    
    // 3. Gradient de la couche de sortie
    double output_delta = output_error * sigmoid_derivative(output_sum);
    
    // 4. Calcul des gradients pour les neurones cachés
    double hidden_delta[2];
    for (int i = 0; i < 2; i++) {
        double error_contribution = output_delta * nn->output_weights[i];
        hidden_delta[i] = error_contribution * 
            sigmoid_derivative(hidden_sum[i]);
    }
    
    // 5. Mise à jour des poids de la couche de sortie
    for (int i = 0; i < 2; i++) {
        nn->output_weights[i] -= nn->learning_rate * output_delta * hidden[i];
    }
    nn->output_bias -= nn->learning_rate * output_delta;
    
    // 6. Mise à jour des poids de la couche cachée
    double inputs[2] = {a, b};
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            nn->hidden_weights[i][j] -= nn->learning_rate * 
                hidden_delta[i] * inputs[j];
        }
        nn->hidden_bias[i] -= nn->learning_rate * hidden_delta[i];
    }
}

// Entraînement du réseau
void train(NeuralNetwork *nn, int simulations, int verbose) 
{
    double inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    double expected[4] = {1, 0, 0, 1};  // XNOR
    
    printf("\n=== Début de l'entraînement ===\n");
    
    for (int simulation = 0; simulation < simulations; simulation++) {
        double total_loss = 0.0;
        
        // Entraîner sur tous les exemples
        for (int i = 0; i < 4; i++) {
            backpropagate(nn, inputs[i][0], inputs[i][1], expected[i]);
            total_loss += compute_loss(nn, inputs[i][0], inputs[i][1], 
                expected[i]);
        }
        
        // Afficher la progression toutes les 1000 simulations
        if (verbose && (simulation % 1000 == 0 || simulation == simulations - 1)) {
            printf("simulation %d: Loss = %.6f\n", simulation, total_loss / 4.0);
        }
    }
    
    printf("=== Entraînement terminé ===\n");
}

// Test de la table de vérité
void test_truth_table(NeuralNetwork *nn) 
{
    printf("\n=== Table de Vérité !A.!B + A.B ===\n");
    printf("A | B | !A.!B | A.B | Attendu | Résultat | Correct\n");
    printf("--|---|-------|-----|---------|----------|--------\n");
    
    double inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    int expected[4] = {1, 0, 0, 1};
    
    int correct_count = 0;
    
    for (int i = 0; i < 4; i++) 
    {
        int a = (int)inputs[i][0];
        int b = (int)inputs[i][1];
        int not_a_not_b = (!a && !b) ? 1 : 0;
        int a_and_b = (a && b) ? 1 : 0;
        int result = predict(nn, inputs[i][0], inputs[i][1]);
        int correct = (result == expected[i]);
        
        if (correct) correct_count++;
        
        printf("%d | %d |   %d   |  %d  |    %d    |    %d     |   %s\n",
            a, b, not_a_not_b, a_and_b, expected[i], 
            result, correct ? "✓" : "✗");
    }
    
    printf("\nPrécision: %d/4 (%.0f%%)\n", correct_count, 
        (correct_count / 4.0) * 100);
}

// Affichage détaillé du calcul
void display_calculation(NeuralNetwork *nn, double a, double b) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    
    printf("\n=== Calcul détaillé pour A=%.0f, B=%.0f ===\n", a, b);
    
    for (int i = 0; i < 2; i++) 
    {
        if (i == 0) 
            printf("\nNeurone caché 1:\n");
        else 
            printf("\nNeurone caché 2:\n");
            
        printf("\tSomme: %.2f * %.0f + %.2f * %.0f + %.2f = %.2f\n",
            nn->hidden_weights[i][0], a, 
            nn->hidden_weights[i][1], b, 
            nn->hidden_bias[i], hidden_sum[i]);
        printf("\tSigmoid(%.2f) = %.4f\n", hidden_sum[i], hidden[i]);
        printf("\tValeur binaire: %d\n", (hidden[i] > 0.5) ? 1 : 0);
    }
    
    printf("\nCouche de sortie:\n");
    printf("\tSomme: %.2f * %.4f + %.2f * %.4f + %.2f = %.2f\n",
        nn->output_weights[0], hidden[0],
        nn->output_weights[1], hidden[1],
        nn->output_bias, output_sum);
    printf("\tSigmoid(%.2f) = %.4f\n", output_sum, output);
    printf("\tRésultat final: %d\n", (output > 0.5) ? 1 : 0);
}

// Affichage de l'architecture
void display_architecture(NeuralNetwork *nn) 
{
    printf("\n=== Architecture du Réseau ===\n");
    printf("\nCouche d'entrée: 2 neurones (A, B)\n");
    printf("\nCouche cachée: 2 neurones\n");
    printf("  Neurone 1:\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->hidden_weights[0][0], 
        nn->hidden_weights[0][1], 
        nn->hidden_bias[0]);
    printf("  Neurone 2:\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->hidden_weights[1][0], 
        nn->hidden_weights[1][1], 
        nn->hidden_bias[1]);
    printf("\nCouche de sortie: 1 neurone\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->output_weights[0], 
        nn->output_weights[1], 
        nn->output_bias);
}

int main() 
{
    NeuralNetwork nn;
    
    printf("=== Réseau Neuronal XNOR avec Apprentissage ===\n");
    printf("2. Réseau qui apprend (poids aléatoires + backpropagation)\n");
    
   
    // Mode apprentissage
    init_network_random(&nn);
    printf("\n=== Mode: Apprentissage ===\n");
    printf("\nPoids AVANT l'entraînement (aléatoires):\n");
    display_architecture(&nn);
    
    printf("\nRésultats AVANT l'entraînement:");
    test_truth_table(&nn);
    
    // Entraînement
    int simulations;
    printf("\nNombre de simulation d'entraînement (entre 1 et 100 000): ");
    scanf("%d", &simulations);
    
    train(&nn, simulations, 1);
    
    printf("\nPoids APRES l'entraînement:\n");
    display_architecture(&nn);
    
    printf("\nRésultats APRES l'entraînement:");
    test_truth_table(&nn);
    
    
    return 0;
}