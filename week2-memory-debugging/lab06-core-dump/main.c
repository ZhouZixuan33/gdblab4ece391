#include <stdio.h>

struct boot_command {
    const char *name;
    int argc;
    const char *argv[3];
};

static void load_boot_command(struct boot_command *cmd) {
    cmd->name = "launch-shell";
    cmd->argc = 3;
    cmd->argv[0] = "shell";
    cmd->argv[1] = NULL;
    cmd->argv[2] = "--verbose";
}

static int checksum_argument(const char *arg) {
    return arg[0];
}

static int checksum_command(const struct boot_command *cmd) {
    int checksum = 0;

    for (int i = 0; i < cmd->argc; i++) {
        checksum += checksum_argument(cmd->argv[i]);
    }

    return checksum;
}

static void dispatch_command(const struct boot_command *cmd) {
    int checksum = checksum_command(cmd);

    printf("Dispatching %s with checksum %d\n", cmd->name, checksum);
}

int main(void) {
    struct boot_command cmd;

    load_boot_command(&cmd);
    printf("Loaded command %s with %d args.\n", cmd.name, cmd.argc);
    dispatch_command(&cmd);

    return 0;
}
