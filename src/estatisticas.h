#include <stdio.h>
#include "alocator.h"
#ifndef ESTATISTICAS_H
#define ESTATISTICAS_H

typedef enum { FIRST_FIT, BEST_FIT, WORST_FIT } Estrategia;

Bloco *buscar_first_fit(Bloco *inicio, size_t tam, size_t *examinados);
Bloco *buscar_best_fit (Bloco *inicio, size_t tam, size_t *examinados);
Bloco *buscar_worst_fit(Bloco *inicio, size_t tam, size_t *examinados);
typedef Bloco *(*FuncaoBusca)(Bloco *, size_t, size_t *);
static FuncaoBusca buscar; /* definida em mem_init conforme a estratégia */

typedef struct {
    size_t total_livre, total_ocupado, maior_livre;
    size_t n_livres, n_ocupados;
    size_t frag_interna; /* soma de tamanho - pedido */
    size_t n_malloc, n_falhas, n_falhas_frag;
    size_t blocos_examinados; /* soma das buscas */
} Estatisticas;

#endif // ESTATISTICAS_H