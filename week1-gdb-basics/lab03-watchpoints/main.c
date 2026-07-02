#include <stdio.h>

struct scheduler_state {
    int ready_count;
    int current_pid;
    int ticks;
};

static struct scheduler_state state = {
    .ready_count = 3,
    .current_pid = 10,
    .ticks = 0,
};

static int ready_slots[3] = {10, 11, 12};
static int tracked_index = 1;
static int *tracked_slot = &ready_slots[1];

static void print_ready_slots(const char *label) {
    printf("%s [%d, %d, %d] tracked_index=%d tracked_value=%d\n",
           label,
           ready_slots[0],
           ready_slots[1],
           ready_slots[2],
           tracked_index,
           *tracked_slot);
}

static void rotate_current_process(void) {
    state.current_pid++;

    if (state.current_pid > 12) {
        state.current_pid = 10;
    }
}

static void cleanup_finished_processes(void) {
    /*
     * Intentional bug: this cleanup path should not run in this toy scenario.
     * It corrupts ready_count and simulates an unexpected global state change.
     */
    if (state.ticks >= 2) {
        state.ready_count--;
    }
}

static void scheduler_tick(void) {
    state.ticks++;
    rotate_current_process();
    cleanup_finished_processes();
}

static void run_ready_count_practice(void) {
    printf("Practice 1: ready_count watchpoint\n");
    printf("Initial ready_count: %d\n", state.ready_count);

    for (int i = 0; i < 4; i++) {
        scheduler_tick();
        printf("tick=%d current_pid=%d ready_count=%d\n",
               state.ticks,
               state.current_pid,
               state.ready_count);
    }

    printf("Expected ready_count to remain 3.\n");
}

static void mark_tracked_slot_not_ready(int *slot) {
    /*
     * Intentional bug: this helper should only inspect the tracked slot.
     * It writes through the pointer and corrupts ready_slots[1].
     */
    *slot = -1;
}

static void rebuild_ready_queue(void) {
    mark_tracked_slot_not_ready(tracked_slot);
}

static void run_pointer_watch_practice(void) {
    printf("\nPractice 2: pointer and array watchpoints\n");
    print_ready_slots("Initial ready_slots:");
    rebuild_ready_queue();
    print_ready_slots("After rebuild_ready_queue:");
    printf("Expected ready_slots[1] to remain 11.\n");
}

int main(void) {
    run_ready_count_practice();
    run_pointer_watch_practice();
    return 0;
}
