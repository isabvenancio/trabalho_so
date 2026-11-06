/* Workload 2: alocações e liberações misturadas, em ordem pseudoaleatória.
 * Cada operação vira uma linha do CSV, para o gráfico de fragmentação externa ao longo do tempo.
 *
 * Este programa também faz dois experimentos do TSO:
 *   - coalescência: repete o workload com a coalescência ligada e desligada;
 *   - escala: repete com vários tamanhos de heap e números de operações. */
#include <stdlib.h> /* rand e srand */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define HEAP 2000
#define OPERACOES 100
#define SEMENTE 42
#define MAX_ATIVOS 1024 /* com folga: cada bloco gasta pelo menos 48 bytes do heap */

/* roda uma execução e grava no CSV com o nome em 'workload'.
 * todas_linhas = 1 grava toda operação; 0 grava só a primeira e a última (o suficiente para a escala) */
void rodar(const char *workload, size_t heap, int operacoes, int todas_linhas) {
    void *ativos[MAX_ATIVOS]; /* blocos alocados e ainda não liberados */
    int n_ativos = 0;

    mem_init(heap);
    for (int operacao = 1; operacao <= operacoes; operacao++) {
        /* os 3 sorteios acontecem em toda operação, mesmo sem usar todos: assim todas
           as estratégias recebem a mesma sequência, mesmo quando uma falha e outra não */
        int sorteio = rand() % 100;
        size_t tamanho = 10 + rand() % 141; /* de 10 a 150 bytes */
        int posicao = rand();

        gravar_csv = todas_linhas || operacao == 1 || operacao == operacoes; //na escala, só a primeira e a última operação viram linha

        if (sorteio < 60 || n_ativos == 0) { /* 60% alocam e 40% liberam: o heap vai enchendo */
            void *p = alocar_e_registrar(workload, operacao, heap, tamanho); //aloca e grava a linha no CSV
            if (p != NULL) { //se falhou, não há bloco para guardar
                ativos[n_ativos] = p;
                n_ativos++;
            }
        } else {
            int escolhido = posicao % n_ativos; //sorteia um dos blocos ainda ativos
            liberar_e_registrar(workload, operacao, heap, ativos[escolhido]);
            ativos[escolhido] = ativos[n_ativos - 1]; /* o último ativo ocupa o lugar do que saiu */
            n_ativos--;
        }
    }
    if (todas_linhas) mem_dump_csv(workload); /* a escala não entra no gráfico de barra do heap */
    mem_destroy();
}

int main(void) {
    size_t heaps[] = { 1000, 5000, 20000 };
    int operacoes[] = { 100, 1000, 10000 };

    /* workload 2 normal e experimento de coalescência: 1 = ligada, 0 = desligada */
    for (coalescencia_ativa = 1; coalescencia_ativa >= 0; coalescencia_ativa--) { //roda com 1 e depois com 0
        for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) { //uma vez para cada estratégia
            srand(SEMENTE); /* mesma semente: todas as estratégias recebem os mesmos pedidos */
            rodar("workload2", HEAP, OPERACOES, 1);
        }
    }

    /* experimento de escala, com a coalescência ligada: 3 heaps x 3 números de operações */
    coalescencia_ativa = 1; //o laço de cima termina com -1: volta para ligada
    for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) {
        for (int h = 0; h < 3; h++) {
            for (int o = 0; o < 3; o++) {
                srand(SEMENTE); //mesma semente: todas as estratégias recebem os mesmos pedidos
                rodar("escala", heaps[h], operacoes[o], 0);
            }
        }
    }
    return 0;
}
