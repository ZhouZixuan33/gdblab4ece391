int check_preserves_s1(void);

volatile int g_check_result;
volatile int g_done;

int main(void) {
    g_check_result = check_preserves_s1();
    g_done = 1;

    return g_check_result;
}
