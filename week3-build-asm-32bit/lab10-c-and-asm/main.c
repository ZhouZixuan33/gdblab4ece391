long asm_add_three(long first, long second, long third);
int check_preserves_s1(void);

volatile long g_add_result;
volatile int g_check_result;
volatile int g_done;

int main(void) {
    g_add_result = asm_add_three(100, 20, 1);
    g_check_result = check_preserves_s1();
    g_done = 1;

    return g_check_result;
}
