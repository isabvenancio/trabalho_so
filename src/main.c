#include <stdio.h>
#include "alocador.h"

int main(void) {
    mem_init(4096);

    void *a = meu_malloc(64);
    void *b = meu_malloc(128);

    if (a != NULL) {
        meu_free(a);
    }

    if (b != NULL) {
        meu_free(b);
    }

    mem_dump();
    return 0;
}
