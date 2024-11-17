#include <Arduino.h>

struct EuclideanParams {
    bool enabled = false;
    int steps = 10;
    int triggers = 6;
    int rotation = 1;
};

// Helper function to distribute the pattern based on counts and remainders
void distributePattern(int level, int counts[], int remainders[], int pattern[], int &index) {
    if (level == -1) {
        pattern[index++] = 0; // Add a rest
    } else if (level == -2) {
        pattern[index++] = 1; // Add a trigger
    } else {
        for (int i = 0; i < counts[level]; i++) {
            distributePattern(level - 1, counts, remainders, pattern, index);
        }
        if (remainders[level] != 0) {
            distributePattern(level - 2, counts, remainders, pattern, index);
        }
    }
}

// Euclidean pattern generation based on Bjorklund's algorithm
void GeneratePattern(EuclideanParams &params, int *rhythm) {
    // Temporary arrays for computation
    int counts[128] = {0};
    int remainders[128] = {0};
    int pattern[128] = {0}; // Temporary pattern storage
    int divisor = params.steps - params.triggers;
    int level = 0;

    // Step 1: Initialize counts and remainders
    remainders[0] = params.triggers;

    while (true) {
        counts[level] = divisor / remainders[level];
        remainders[level + 1] = divisor % remainders[level];
        divisor = remainders[level];
        level++;
        if (remainders[level] <= 1)
            break;
    }

    counts[level] = divisor;

    // Step 2: Distribute triggers and rests
    int index = 0;
    distributePattern(level, counts, remainders, pattern, index);

    // Step 3: Rotate the pattern
    for (int i = 0; i < params.steps; i++) {
        rhythm[(i + params.rotation) % params.steps] = pattern[i];
    }
}
