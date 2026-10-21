#include <stdio.h>
#ifndef ALOCADOR_H
#define ALOCADOR_H
#define ALINHAMENTO 8
#define ALINHAR(n) (((n) + (ALINHAMENTO - 1)) & ~(size_t)(ALINHAMENTO - 1))
#define TAM_CAB ALINHAR(sizeof(Bloco))
#define MIN_DADOS 8 /* menor área de dados que vale a pena criar */

typedef struct bloco {
    size_t tamanho;
    size_t pedido;/* quanto o usuário pediu (stats) */
    int livre;
    struct bloco *proximo;
    struct bloco *anterior;
} Bloco;

typedef enum status_aloc {
    ALOC_OK = 0,
    ERR_HEAP_NOT_INITIALIZED,
    ERR_OUT_OF_MEMORY,     /* não há espaço livre suficiente nem somando tudo */
    ERR_FRAGMENTATION,     /* há espaço livre total, mas nenhum bloco contíguo */
    ERR_BAD_POINTER,
    ERR_DOUBLE_FREE
} AlocStatus;

extern int coalescencia_ativa; /* 1 = funde vizinhos livres no free; 0 = não funde */
extern char *heap_base;        /* início do heap simulado (NULL antes do mem_init) */

void mem_init(size_t heap_size);
void mem_destroy(void);
void* meu_malloc(size_t tamanho);
void meu_free(void* ptr);
void mem_stats(void);
void mem_dump(void);

#endif // ALOCADOR_H
