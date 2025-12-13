#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Sigmoid activation function
double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

// Sigmoid derivative (needed for backpropagation)
double sigmoid_derivative(double x)
{
    double sig = sigmoid(x);
    return sig * (1.0 - sig);
}

// Neural network structure for !A.!B + A.B
typedef struct 
{
    // Hidden layer: 2 neurons to compute !A.!B and A.B
    double hidden_weights[2][2];  // [neuron][input]
    double hidden_bias[2];
    
    // Output layer: combines both results
    double output_weights[2];
    double output_bias;
    
    // Learning rate
    double learning_rate;
} NeuralNetwork;

// RANDOM initialization of the network (for training)
void init_network_random(NeuralNetwork *nn)
{
    srand(time(NULL));
    
    // Random initialization between -1 and 1
    for (int i = 0; i < 2; i++) 
    {
        for (int j = 0; j < 2; j++) 
        {
            nn->hidden_weights[i][j] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        }
        nn->hidden_bias[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        nn->output_weights[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    }
    nn->output_bias = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    
    nn->learning_rate = 0.5;  // Learning rate
}



// Forward propagation 
void forward(NeuralNetwork *nn, double a, double b, double *hidden, 
    double *hidden_sum, double *output, double *output_sum)
{
    // Hidden layer: compute both neurons
    for (int i = 0; i < 2; i++)
    {
        hidden_sum[i] = nn->hidden_weights[i][0] * a + 
                        nn->hidden_weights[i][1] * b + 
                        nn->hidden_bias[i];
        hidden[i] = sigmoid(hidden_sum[i]);
    }
    
    // Output layer
    *output_sum = nn->output_weights[0] * hidden[0] + 
                  nn->output_weights[1] * hidden[1] + 
                  nn->output_bias;
    *output = sigmoid(*output_sum);
}

// Prediction (0 or 1)
int predict(NeuralNetwork *nn, double a, double b) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    return (output > 0.5) ? 1 : 0;
}

// Compute loss (error)
double compute_loss(NeuralNetwork *nn, double a, double b, 
    double expected) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    double error = output - expected;
    return error * error;  // Error
}

// BACKPROPAGATION: The heart of learning 
void backpropagate(NeuralNetwork *nn, double a, double b, 
    double expected)
{
    double hidden[2], hidden_sum[2], output, output_sum;
    
    // 1. Forward pass to get all values
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    
    // 2. Compute output error
    double output_error = output - expected;
    
    // 3. Gradient of the output layer
    double output_delta = output_error * sigmoid_derivative(output_sum);
    
    // 4. Compute gradients for hidden neurons
    double hidden_delta[2];
    for (int i = 0; i < 2; i++) 
    {
        double error_contribution = output_delta * nn->output_weights[i];
        hidden_delta[i] = error_contribution * 
            sigmoid_derivative(hidden_sum[i]);
    }
    
    // 5. Update output layer weights
    for (int i = 0; i < 2; i++) 
    {
        nn->output_weights[i] -= nn->learning_rate * output_delta * hidden[i];
    }
    nn->output_bias -= nn->learning_rate * output_delta;
    
    // 6. Update hidden layer weights
    double inputs[2] = {a, b};
    for (int i = 0; i < 2; i++) 
    {
        for (int j = 0; j < 2; j++) 
        {
            nn->hidden_weights[i][j] -= nn->learning_rate * 
                hidden_delta[i] * inputs[j];
        }
        nn->hidden_bias[i] -= nn->learning_rate * hidden_delta[i];
    }
}

// Train the network
void train(NeuralNetwork *nn, int simulations, int verbose) 
{
    double inputs[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
    double expected[4] = {1, 0, 0, 1};  // XNOR
    
    printf("\n=== Training Started ===\n");
    
    for (int simulation = 0; simulation < simulations; simulation++) 
    {
        double total_loss = 0.0;
        
        // Train on all examples
        for (int i = 0; i < 4; i++) 
        {
            backpropagate(nn, inputs[i][0], inputs[i][1], expected[i]);
            total_loss += compute_loss(nn, inputs[i][0], inputs[i][1], 
                expected[i]);
        }
        
        // Display progress every 1000 simulations
        if (verbose && (simulation % 1000 == 0 || simulation == simulations - 1)) 
        {
            printf("Simulation %d: Loss = %.6f\n", simulation, total_loss / 4.0);
        }
    }
    
    printf("=== Training Completed ===\n");
}

// Test the truth table
void test_truth_table(NeuralNetwork *nn) 
{
    printf("\n=== Truth Table !A.!B + A.B ===\n");
    printf("A | B | !A.!B | A.B | Expected | Result | Correct\n");
    printf("--|---|-------|-----|----------|--------|--------\n");
    
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
        
        printf("%d | %d |   %d   |  %d  |    %d     |   %d    |   %s\n",
            a, b, not_a_not_b, a_and_b, expected[i], 
            result, correct ? "✓" : "✗");
    }
    
    printf("\nAccuracy: %d/4 (%.0f%%)\n", correct_count, 
        (correct_count / 4.0) * 100);
}

// Detailed calculation display
void display_calculation(NeuralNetwork *nn, double a, double b) 
{
    double hidden[2], hidden_sum[2], output, output_sum;
    
    forward(nn, a, b, hidden, hidden_sum, &output, &output_sum);
    
    printf("\n=== Detailed Calculation for A=%.0f, B=%.0f ===\n", a, b);
    
    for (int i = 0; i < 2; i++) 
    {
        if (i == 0) 
            printf("\nHidden Neuron 1:\n");
        else 
            printf("\nHidden Neuron 2:\n");
            
        printf("\tSum: %.2f * %.0f + %.2f * %.0f + %.2f = %.2f\n",
            nn->hidden_weights[i][0], a, 
            nn->hidden_weights[i][1], b, 
            nn->hidden_bias[i], hidden_sum[i]);
        printf("\tSigmoid(%.2f) = %.4f\n", hidden_sum[i], hidden[i]);
        printf("\tBinary value: %d\n", (hidden[i] > 0.5) ? 1 : 0);
    }
    
    printf("\nOutput Layer:\n");
    printf("\tSum: %.2f * %.4f + %.2f * %.4f + %.2f = %.2f\n",
        nn->output_weights[0], hidden[0],
        nn->output_weights[1], hidden[1],
        nn->output_bias, output_sum);
    printf("\tSigmoid(%.2f) = %.4f\n", output_sum, output);
    printf("\tFinal result: %d\n", (output > 0.5) ? 1 : 0);
}

// Display architecture
void display_architecture(NeuralNetwork *nn) 
{
    printf("\n=== Network Architecture ===\n");
    printf("\nInput layer: 2 neurons (A, B)\n");
    printf("\nHidden layer: 2 neurons\n");
    printf("  Neuron 1:\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->hidden_weights[0][0], 
        nn->hidden_weights[0][1], 
        nn->hidden_bias[0]);
    printf("  Neuron 2:\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->hidden_weights[1][0], 
        nn->hidden_weights[1][1], 
        nn->hidden_bias[1]);
    printf("\nOutput layer: 1 neuron\n");
    printf("\tw1=%.4f, w2=%.4f, bias=%.4f\n",
        nn->output_weights[0], 
        nn->output_weights[1], 
        nn->output_bias);
}

int main() 
{
    NeuralNetwork nn;
    
    printf("=== XNOR Neural Network with Learning ===\n");
    printf("2. Network that learns (random weights + backpropagation)\n");
    
   
    
    init_network_random(&nn);
    printf("\n=== Mode: Learning ===\n");
    printf("\nWeights BEFORE training (random):\n");
    display_architecture(&nn);
    
    printf("\nResults BEFORE training:");
    test_truth_table(&nn);
    
    // Training
    int simulations;
    printf("\nNumber of training simulations (between 1 and 100,000): ");
    scanf("%d", &simulations);
    
    train(&nn, simulations, 1);
    
    printf("\nWeights AFTER training:\n");
    display_architecture(&nn);
    
    printf("\nResults AFTER training:");
    test_truth_table(&nn);
    
    
    return 0;
}