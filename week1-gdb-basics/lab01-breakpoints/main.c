#include <stdio.h>

static int sum_scores(const int *scores, int count) {
    int total = 0;

    for (int i = 0; i < count; i++) {
        total += scores[i];
    }

    return total;
}

static double average_score(const int *scores, int count) {
    int total = sum_scores(scores, count);

    /* Intentional bug: the denominator should be count, not count - 1. */
    return (double)total / (count - 1);
}

int main(void) {
    int scores[] = {80, 90, 85, 95};
    int count = (int)(sizeof(scores) / sizeof(scores[0]));
    double average = average_score(scores, count);

    printf("Average score: %.2f\n", average);
    printf("Expected: 87.50\n");

    return 0;
}
