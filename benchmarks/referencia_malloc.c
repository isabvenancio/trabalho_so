/* Referência (opcional do TSO): compara o tempo do nosso alocador com o malloc/free reais
 * da biblioteca do C, usando o mesmo padrão de pedidos do workload 2.
 * O heap é grande o bastante para nenhum pedido falhar: assim todos fazem exatamente as
 * mesmas operações. Cada operação vira uma linha: quem,operacao,tempo_ns */
#include <stdio.h>
#include <stdlib.h> /* malloc, free, rand e srand */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define HEAP (1024 * 1024) /* 1 MiB */
#define OPERACOES 2000
#define SEMENTE 42

/* usar_malloc_real = 1 usa malloc/free da biblioteca; 0 usa meu_malloc/meu_free */
void rodar(const char *nome, int usar_malloc_real) {
    void *ativos[OPERACOES]; /* blocos alocados e ainda não liberados */
    int n_ativos = 0;

    srand(SEMENTE); /* mesma semente: todos recebem os mesmos pedidos */
    for (int operacao = 1; operacao <= OPERACOES; operacao++) {
        int sorteio = rand() % 100;
        size_t tamanho = 10 + rand() % 141; /* de 10 a 150 bytes */
        int posicao = rand();
        long inicio, tempo;

        if (sorteio < 60 || n_ativos == 0) {
            void *p;
            inicio = agora_ns();
            if (usar_malloc_real) p = malloc(tamanho);
            else                  p = meu_malloc(tamanho);
            tempo = agora_ns() - inicio;

            ativos[n_ativos] = p;
            n_ativos++;
            printf("%s,malloc,%ld\n", nome, tempo);
        } else {
            int escolhido = posicao % n_ativos;
            inicio = agora_ns();
            if (usar_malloc_real) free(ativos[escolhido]);
            else                  meu_free(ativos[escolhido]);
            tempo = agora_ns() - inicio;

            ativos[escolhido] = ativos[n_ativos - 1]; /* o último ativo ocupa o lugar do que saiu */
            n_ativos--;
            printf("%s,free,%ld\n", nome, tempo);
        }
    }

    if (usar_malloc_real) { /* o malloc real precisa liberar um por um; o nosso heap sai inteiro no mem_destroy */
        for (int i = 0; i < n_ativos; i++) free(ativos[i]);
    }
}

int main(void) {
    printf("quem,operacao,tempo_ns\n");
    rodar("malloc_real", 1);

    for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) {
        mem_init(HEAP);
        rodar(nomes_estrategias[estrategia_atual], 0);
        mem_destroy();
    }
    return 0;
}
