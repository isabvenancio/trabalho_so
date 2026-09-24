#include <stdio.h>
#include <stdlib.h> /* malloc e free */
#include <string.h> /* string */
#include "alocator.h"

/*
git status
git add .
git commit -m "descricao da alteracao"
git push
*/

/* funções principais */
void mem_init(size_t heap_size){
    /* inicializa o heap com o tamanho e o primeiro bloco livre */
}

void* meu_malloc(size_t tamanho){
    /* coordena alinhamento, busca, splitting*/
    /* retorna ponteiro para o bloco alocado */
}

void meu_free(void* ptr){
    /*libera e faz coalescencia */
    /* verifica se o ponteiro é válido */
}

void mem_stats(void){
    /* exibe estatísticas da memória */
}

void mem_dump(void){
    /* exibe o estado atual da memória (blocos atuais)*/
}


/* funções auxiliares */
Bloco* buscar_first_fit(size_t tamanho){

}

Bloco* buscar_best_fit(size_t tamanho){

}

Bloco* buscar_worst_fit(size_t tamanho){

}

void splitting(Bloco* bloco, size_t tamanho){

}

void coalescencia(Bloco* bloco){

}