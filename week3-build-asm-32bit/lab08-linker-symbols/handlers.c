#include "handlers.h"

struct handler_status install_keyboard_handler(void) {
    struct handler_status status = {
        .vector = 0x21,
        .name = "keyboard",
        .installed = 1,
    };

    return status;
}
