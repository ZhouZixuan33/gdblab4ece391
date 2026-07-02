#include <stdio.h>

#include "handlers.h"

static void print_handler_status(struct handler_status status) {
    printf("handler name: %s\n", status.name);
    printf("interrupt vector: 0x%x\n", status.vector);
    printf("installed: %s\n", status.installed ? "yes" : "no");
}

int main(void) {
    struct handler_status keyboard = install_keyboard_handler();

    printf("Installing boot-time handlers...\n");
    print_handler_status(keyboard);

    return 0;
}
