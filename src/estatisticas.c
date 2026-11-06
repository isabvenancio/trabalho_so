#define _POSIX_C_SOURCE 199309L /* libera o clock_gettime quando se compila com -std=c11 */

#include <stdio.h>
#include <time.h> /* clock_gettime */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

Estatisticas estatisticas = {0}; // inicializa todas as estatísticas com zero
int gravar_csv = 1; //1 = grava uma linha por operação; a escala do workload 2 desliga às vezes

/* lê o relógio do sistema, em nanossegundos */
long agora_ns(void) {
    struct timespec t; //segundos + nanossegundos
    clock_gettime(CLOCK_MONOTONIC_RAW, &t); //relógio que só anda para frente: não é afetado se a hora do sistema mudar
    return t.tv_sec * 1000000000L + t.tv_nsec; //converte tudo para nanossegundos
}

/* imprime uma linha do CSV com a situação atual do heap.
 * O make bench manda essa saída para resultados/comparativo.csv.
 * As colunas estão no cabeçalho, no alvo bench do Makefile: a ordem tem que ser a mesma.
 * Um tempo igual a -1 significa "esta operação não tem esse tempo" e fica vazio no CSV. */
static void registrar_csv(const char *workload, int operacao, size_t heap_size,
                          long tempo_alloc_ns, long tempo_free_ns) {
    if (!gravar_csv) return; //esta operação não deve virar linha

    contar_blocos(); /* atualiza maior_livre, total_livre, n_livres e n_ocupados */

    printf("%s,%s,%d,%d,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,",
           workload, nomes_estrategias[estrategia_atual], coalescencia_ativa, operacao, heap_size,
           estatisticas.n_malloc - estatisticas.n_falhas, /* alocações que deram certo */
           estatisticas.n_falhas, estatisticas.n_falhas_frag,
           estatisticas.maior_livre, estatisticas.total_livre, estatisticas.frag_interna,
           (estatisticas.n_livres + estatisticas.n_ocupados) * TAM_CAB, /* overhead: um cabeçalho por bloco */
           estatisticas.blocos_examinados);

    if (tempo_alloc_ns >= 0) printf("%ld", tempo_alloc_ns); //os tempos vão nas duas últimas colunas
    printf(",");
    if (tempo_free_ns >= 0) printf("%ld", tempo_free_ns);
    printf("\n");
}

/* meu_malloc que também mede o tempo e grava a linha no CSV; devolve o mesmo que o meu_malloc */
void *alocar_e_registrar(const char *workload, int operacao, size_t heap_size, size_t tamanho) {
    long inicio = agora_ns(); //relógio antes
    void *p = meu_malloc(tamanho);
    long tempo = agora_ns() - inicio; //relógio depois menos antes = quanto o meu_malloc demorou

    /* só guarda o tempo das alocações que deram certo: a falha imprime uma mensagem, o que atrasaria a medida */
    registrar_csv(workload, operacao, heap_size, p != NULL ? tempo : -1, -1);
    return p;
}

/* meu_free que também mede o tempo e grava a linha no CSV */
void liberar_e_registrar(const char *workload, int operacao, size_t heap_size, void *ptr) {
    long inicio = agora_ns();
    meu_free(ptr);
    long tempo = agora_ns() - inicio;

    registrar_csv(workload, operacao, heap_size, -1, tempo);
}
