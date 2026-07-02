#ifndef HANDLERS_H
#define HANDLERS_H

struct handler_status {
    int vector;
    const char *name;
    int installed;
};

struct handler_status install_keyboard_handler(void);

#endif
