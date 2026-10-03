#include <stdio.h>
#include "alocador.h"
#ifndef ESTATISTICAS_H
#define ESTATISTICAS_H

typedef struct {
    size_t total_livre, total_ocupado, maior_livre;
    size_t n_livres, n_ocupados;
    size_t frag_interna; /* soma de tamanho - pedido */
    size_t n_malloc, n_falhas, n_falhas_frag;
    size_t blocos_examinados; /* soma das buscas */
} Estatisticas;

#endif // ESTATISTICAS_H