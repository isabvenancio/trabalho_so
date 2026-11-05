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

long agora_ns(void); /* relógio: para medir, guarda o valor antes da operação e subtrai depois */

extern int gravar_csv; /* 1 = cada operação grava uma linha no CSV; 0 = não grava */

/* fazem a operação, medem o tempo dela e imprimem uma linha do CSV */
void *alocar_e_registrar(const char *workload, int operacao, size_t heap_size, size_t tamanho);
void liberar_e_registrar(const char *workload, int operacao, size_t heap_size, void *ptr);

#endif // ESTATISTICAS_H

