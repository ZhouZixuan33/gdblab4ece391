#include <stdio.h>

struct boot_request {
    int arg0;
    int arg1;
    int arg2;
};

__attribute__((noinline))
int encode_triple(int first, int second, int third) {
    /* Local stack storage makes esp and ebp visibly different in GDB. */
    volatile int scratch[4];
    int encoded = first + (second * 10) + (third * 100);

    scratch[0] = first + second;
    scratch[1] = second + third;
    scratch[2] = encoded;
    scratch[3] = scratch[0] + scratch[1];

    return scratch[2];
}

static int dispatch_request(const struct boot_request *request) {
    /* Intentional bug: arg1 and arg2 are swapped at the call site. */
    return encode_triple(request->arg0, request->arg2, request->arg1);
}

int main(void) {
    struct boot_request request = {
        .arg0 = 1,
        .arg1 = 2,
        .arg2 = 3,
    };
    int expected = encode_triple(request.arg0, request.arg1, request.arg2);
    int actual = dispatch_request(&request);

    printf("Request args: arg0=%d arg1=%d arg2=%d\n", request.arg0, request.arg1, request.arg2);
    printf("Expected encode_triple(arg0, arg1, arg2): %d\n", expected);
    printf("Actual dispatched encoding: %d\n", actual);
    printf("Expected the dispatcher to preserve argument order.\n");

    return 0;
}
