#ifndef LAB15_PLIC_H
#define LAB15_PLIC_H

typedef unsigned long plic_u64;

void plic_enable_uart(void);
plic_u64 plic_claim(void);
void plic_complete(plic_u64 source);

#endif

