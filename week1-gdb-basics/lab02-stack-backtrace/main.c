#include <stdio.h>
#include <string.h>

struct user {
    char name[16];
    int id;
};

static void copy_name(struct user *u, const char *input) {
    strcpy(u->name, input);
}

static void parse_user(struct user *u, const char *raw_name) {
    copy_name(u, raw_name);
    u->id = 391;
}

static void load_user_from_args(int argc, char **argv, struct user *u) {
    const char *name = NULL;

    if (argc > 1) {
        name = argv[1];
    }

    parse_user(u, name);
}

int main(int argc, char **argv) {
    struct user current;

    load_user_from_args(argc, argv, &current);
    printf("Loaded user %s with id %d\n", current.name, current.id);

    return 0;
}
