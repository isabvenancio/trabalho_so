#ifndef ESTATISTICAS_H
#define ESTATISTICAS_H

#include <stddef.h> /* size_t */

// Acumulados ao longo da execução: n_malloc, n_falhas, n_falhas_frag, blocos_examinados e frag_interna.
// Situação atual, recalculada por contar_blocos(): total_livre, total_ocupado, maior_livre, n_livres e n_ocupados.
typedef struct {
    size_t total_livre, total_ocupado, maior_livre;
    size_t n_livres, n_ocupados;
    size_t frag_interna; /* soma de tamanho - pedido */
    size_t n_malloc, n_falhas, n_falhas_frag;
    size_t blocos_examinados; /* soma das buscas */
} Estatisticas;

extern Estatisticas estatisticas; // variável global para armazenar as estatísticas da memória

#endif // ESTATISTICAS_H

