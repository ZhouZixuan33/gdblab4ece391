#include <stdio.h>

int check_preserves_ebx(void);

int main(void) {
    int check_result = check_preserves_ebx();

    printf("Mixed C/assembly register preservation check\n");
    printf("check_preserves_ebx returned: %d\n", check_result);

    if (check_result == 0) {
        printf("PASS: broken_helper preserved ebx.\n");
    } else {
        printf("FAIL: broken_helper changed ebx without restoring it.\n");
        printf("Expected callee-saved registers to survive a function call.\n");
    }

    return 0;
}
