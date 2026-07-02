#include <stdio.h>

struct grade_packet {
    int scores[4];
    int guard_word;
    int student_id;
};

static void write_score(int *scores, int index, int value) {
    scores[index] = value;
}

static void import_scores(struct grade_packet *packet) {
    int incoming_scores[] = {80, 90, 85, 95, 100};
    int incoming_count = (int)(sizeof(incoming_scores) / sizeof(incoming_scores[0]));

    /*
     * Intentional bug: packet->scores has only 4 elements, but this loop
     * imports 5 values. scores[4] overwrites packet->guard_word.
     */
    for (int i = 0; i < incoming_count; i++) {
        write_score(packet->scores, i, incoming_scores[i]);
    }
}

static void print_packet(const char *label, const struct grade_packet *packet) {
    printf("%s\n", label);
    printf("scores: [%d, %d, %d, %d]\n",
           packet->scores[0],
           packet->scores[1],
           packet->scores[2],
           packet->scores[3]);
    printf("guard_word: 0x%x\n", packet->guard_word);
    printf("student_id: %d\n", packet->student_id);
}

int main(void) {
    struct grade_packet packet = {
        .scores = {0, 0, 0, 0},
        .guard_word = 0x39139139,
        .student_id = 391,
    };

    print_packet("Before import:", &packet);
    import_scores(&packet);
    print_packet("\nAfter import:", &packet);
    printf("Expected guard_word to stay 0x39139139.\n");

    return 0;
}
