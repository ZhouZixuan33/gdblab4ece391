struct boot_request {
    int arg0;
    int arg1;
    int arg2;
};

volatile int g_expected;
volatile int g_actual;
volatile int g_done;

__attribute__((noinline))
int encode_triple(int first, int second, int third) {
    volatile int scratch[4];
    int encoded = first + (second * 10) + (third * 100);

    scratch[0] = first + second;
    scratch[1] = second + third;
    scratch[2] = encoded;
    scratch[3] = scratch[0] + scratch[1];

    return scratch[2];
}

static int dispatch_request(const struct boot_request *request) {
    return encode_triple(request->arg0, request->arg2, request->arg1);
}

int main(void) {
    struct boot_request request = {
        .arg0 = 1,
        .arg1 = 2,
        .arg2 = 3,
    };

    g_expected = encode_triple(request.arg0, request.arg1, request.arg2);
    g_actual = dispatch_request(&request);
    g_done = 1;

    return g_actual == g_expected ? 0 : 1;
}
