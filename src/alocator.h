#include <stdio.h>
#ifndef ALOCADOR_H
#define ALOCADOR_H

typedef struct bloco {
    size_t tamanho;
    int livre;
    char padding[4];
    struct bloco* proximo;
    struct bloco* anterior;
} Bloco;

typedef enum status_aloc {
    ALOC_SUCESSO,
    ALOC_ERRO_MEMORIA_INSUFICIENTE,
    ALOC_ERRO_BLOCO_INVALIDO,
    ALOC_ERRO_FRAGMENTACAO
} AlocStatus;

void mem_init(size_t heap_size);
void* mem_malloc(size_t tamanho);
void meu_free(void* ptr);
void mem_stats(void);
void mem_dump(void);

#endif // ALOCADOR_H
