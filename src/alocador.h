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
    ALOC_SUCESSO,
    ALOC_ERRO_MEMORIA_INSUFICIENTE,
    ALOC_ERRO_BLOCO_INVALIDO,
    ALOC_ERRO_FRAGMENTACAO
} AlocStatus;

void meu_init(size_t heap_size);
void* meu_malloc(size_t tamanho);
void meu_free(void* ptr);
void mem_stats(void);
void mem_dump(void);

#endif // ALOCADOR_H
