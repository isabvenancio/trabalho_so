/* Demonstração mínima do alocador: é o programa que o make gera (./alocador). */
#include <stdio.h>
#include "alocador.h"

int main(void) {
    mem_init(4096); //heap de 4096 bytes: um único bloco livre

    void *a = meu_malloc(64);  //vira um bloco de 64 bytes
    void *b = meu_malloc(128); //vira um bloco de 128 bytes, logo depois do a

    meu_free(a); //meu_free(NULL) não faz nada, então não precisa testar se deu certo
    meu_free(b); //b se funde com a e com o resto: o heap volta a ser um bloco só

    mem_dump(); //mostra: um único bloco livre de 4056 bytes
    mem_destroy(); //devolve o heap ao sistema
    return 0;
}
