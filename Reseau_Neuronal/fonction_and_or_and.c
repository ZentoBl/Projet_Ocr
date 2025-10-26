#include <stdio.h>
#include <math.h>

// Fonction d'activation sigmoid
double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
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
} NeuralNetwork;

// Initialisation du réseau pour !A.!B + A.B
void init_network(NeuralNetwork *nn) 
{
    // Neurone caché 1 : calcule A.B (AND)
    nn->hidden_weights[0][0] = 1.0;   // Poids pour A
    nn->hidden_weights[0][1] = 1.0;   // Poids pour B
    nn->hidden_bias[0] = -1.5;        // Biais pour AND
    
    // Neurone caché 2 : calcule !A.!B (NOR)
    nn->hidden_weights[1][0] = -1.0;  // Poids négatif pour !A
    nn->hidden_weights[1][1] = -1.0;  // Poids négatif pour !B
    nn->hidden_bias[1] = 0.5;         // Biais pour NOR (actif si A=0 et B=0)
    
    // Couche de sortie : OR des deux neurones (!A.!B + A.B)
    nn->output_weights[0] = 1.0;
    nn->output_weights[1] = 1.0;
    nn->output_bias = -0.5;
}

// Propagation avant (forward pass)
void forward(NeuralNetwork *nn, int a, int b, double *hidden, double *output) 
{
    // Couche cachée : calcul des deux neurones
    for (int i = 0; i < 2; i++) 
    {
        double sum = nn->hidden_weights[i][0] * a + 
                     nn->hidden_weights[i][1] * b + 
                     nn->hidden_bias[i];
        hidden[i] = sigmoid(sum * 5.0);
    }
    
    // Couche de sortie : combine les deux résultats (OR)
    double sum = nn->output_weights[0] * hidden[0] + 
                 nn->output_weights[1] * hidden[1] + 
                 nn->output_bias;
    *output = sigmoid(sum * 5.0);
}

// Prédiction (0 ou 1)
int predict(NeuralNetwork *nn, int a, int b) 
{
    double hidden[2];
    double output;
    forward(nn, a, b, hidden, &output);
    return (output > 0.5) ? 1 : 0;
}

// Test de la table de vérité
void test_truth_table(NeuralNetwork *nn) 
{
    printf("\n=== Table de Vérité !A.!B + A.B ===\n");
    printf("A | B | !A.!B | A.B | Attendu | Résultat | Correct\n");
    printf("--|---|-------|-----|---------|----------|--------\n");
    
    int inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    int expected[4] = {1, 0, 0, 1};  // XNOR: vrai si A==B
    
    for (int i = 0; i < 4; i++) 
    {
        int a = inputs[i][0];
        int b = inputs[i][1];
        int not_a_not_b = (!a && !b) ? 1 : 0;
        int a_and_b = (a && b) ? 1 : 0;
        int result = predict(nn, a, b);
        int correct = (result == expected[i]);
        
        printf("%d | %d |   %d   |  %d  |    %d    |    %d     |   %s\n", 
               a, b, not_a_not_b, a_and_b, expected[i], result, correct ? "✓" : "✗");
    }
}

// Affichage détaillé du calcul avec les deux chemins
void display_calculation(NeuralNetwork *nn, int a, int b) 
{
    double hidden[2];
    double output;
    
    printf("\n=== Calcul détaillé pour A=%d, B=%d ===\n", a, b);
    
    // Calcul des neurones cachés
    for (int i = 0; i < 2; i++) 
    {
        double sum = nn->hidden_weights[i][0] * a + 
                     nn->hidden_weights[i][1] * b + 
                     nn->hidden_bias[i];
        hidden[i] = sigmoid(sum * 5.0);
        
        if (i == 0) 
        {
            printf("\nNeurone caché 1 (A.B - AND) :\n");
        } else 
        {
            printf("\nNeurone caché 2 (!A.!B - NOR) :\n");
        }
        printf("\tSomme: %.2f * %d + %.2f * %d + %.2f = %.2f\n",
               nn->hidden_weights[i][0], a, 
               nn->hidden_weights[i][1], b, 
               nn->hidden_bias[i], sum);
        printf("\tSigmoid(%.2f * 5.0) = %.4f\n", sum, hidden[i]);
        printf("\tValeur binaire: %d\n", (hidden[i] > 0.5) ? 1 : 0);
    }
    
    // Calcul de la sortie (OR)
    double sum = nn->output_weights[0] * hidden[0] + 
                 nn->output_weights[1] * hidden[1] + 
                 nn->output_bias;
    output = sigmoid(sum * 5.0);
    
    printf("\nCouche de sortie (!A.!B + A.B) :\n");
    printf("\tSomme: %.2f * %.4f + %.2f * %.4f + %.2f = %.2f\n",
           nn->output_weights[0], hidden[0],
           nn->output_weights[1], hidden[1],
           nn->output_bias, sum);
    printf("\tSigmoid(%.2f * 5.0) = %.4f\n", sum, output);
    printf("\tRésultat final (1 ou 0 en entier): %d\n", (output > 0.5) ? 1 : 0);
    printf("\n!A.!B + A.B = %d\n", (output > 0.5) ? 1 : 0);
}

// Affichage de l'architecture du réseau
void display_architecture(NeuralNetwork *nn) 
{
    printf("\n=== Architecture du Réseau ===\n");
    printf("\nCouche d'entrée: 2 neurones (A, B)\n");
    printf("\nCouche cachée: 2 neurones\n");
    printf("  Neurone 1 (A.B - AND):\n");
    printf("\tw1=%.2f, w2=%.2f, bias=%.2f\n",
           nn->hidden_weights[0][0], nn->hidden_weights[0][1], nn->hidden_bias[0]);
    printf("  Neurone 2 (!A.!B - NOR):\n");
    printf("\tw1=%.2f, w2=%.2f, bias=%.2f\n",
           nn->hidden_weights[1][0], nn->hidden_weights[1][1], nn->hidden_bias[1]);
    printf("\nCouche de sortie: 1 neurone (OR)\n");
    printf("\tw1=%.2f, w2=%.2f, bias=%.2f\n",
           nn->output_weights[0], nn->output_weights[1], nn->output_bias);
}

int main() 
{
    NeuralNetwork nn;
    
    // Initialiser le réseau
    init_network(&nn);
    
    printf("=== Réseau Neuronal pour !A.!B + A.B (XNOR) ===\n");
    printf("(Neurone 1 calcule A.B, Neurone 2 calcule !A.!B, puis OR)\n");
    
    display_architecture(&nn);
    
    // Tester la table de vérité complète
    test_truth_table(&nn);
    
    // Test interactif
    printf("\n\n=== Test personnalisé ===\n");
    int a, b;
    printf("Entrez A (0 ou 1): ");
    scanf("%d", &a);
    printf("Entrez B (0 ou 1): ");
    scanf("%d", &b);
    
    if ((a == 0 || a == 1) && (b == 0 || b == 1)) {
        display_calculation(&nn, a, b);
    } else {
        printf("Erreur: A et B doivent être 0 ou 1\n");
    }
    
    return 0;
}