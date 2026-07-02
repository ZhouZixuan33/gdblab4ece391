#include <stdio.h>

#include "config.h"

static int tasks_to_admit(int requested_tasks) {
    if (requested_tasks > MAX_TASKS) {
        return MAX_TASKS;
    }

    return requested_tasks;
}

int main(void) {
    int requested_tasks = 6;
    int admitted_tasks = tasks_to_admit(requested_tasks);

    printf("Scheduler configuration\n");
    printf("MAX_TASKS compiled into this binary: %d\n", MAX_TASKS);
    printf("SCHEDULER_QUANTUM_MS: %d\n", SCHEDULER_QUANTUM_MS);
    printf("Requested tasks: %d\n", requested_tasks);
    printf("Admitted tasks: %d\n", admitted_tasks);
    printf("If you changed config.h but this output did not change, suspect a stale object file.\n");

    return 0;
}
