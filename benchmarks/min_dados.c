/* Experimento do MIN_DADOS (pergunta 5 do relatório): qual o efeito da menor sobra que vira um bloco novo?
 * Usa o mesmo padrão de pedidos do workload 2, com vários valores de min_dados e várias sementes.
 * Cada execução vira uma linha do CSV; o analise.py calcula a média das sementes. */
#include <stdio.h>
#include <stdlib.h> /* rand e srand */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define HEAP 2000
#define OPERACOES 1000
#define SEMENTES 30
#define MAX_ATIVOS 1024 /* com folga: cada bloco gasta pelo menos 48 bytes do heap */

/* uma execução do padrão do workload 2, com o min_dados e a estratégia atuais; imprime uma linha do CSV */
void rodar(int semente) {
    void *ativos[MAX_ATIVOS]; /* blocos alocados e ainda não liberados */
    int n_ativos = 0;
    double soma_frag_interna = 0, soma_frag_externa = 0, soma_livres = 0; /* somadas a cada operação para tirar a média */

    srand(semente); //cada semente gera uma sequência de pedidos diferente
    mem_init(HEAP);
    for (int operacao = 1; operacao <= OPERACOES; operacao++) {
        int sorteio = rand() % 100;
        size_t tamanho = 10 + rand() % 141; /* de 10 a 150 bytes */
        int posicao = rand();

        if (sorteio < 60 || n_ativos == 0) { /* 60% alocam e 40% liberam, como no workload 2 */
            void *p = meu_malloc(tamanho);
            if (p != NULL) {
                ativos[n_ativos] = p;
                n_ativos++;
            }
        } else {
            int escolhido = posicao % n_ativos; //sorteia um dos blocos ainda ativos
            meu_free(ativos[escolhido]);
            ativos[escolhido] = ativos[n_ativos - 1]; /* o último ativo ocupa o lugar do que saiu */
            n_ativos--;
        }

        contar_blocos(); //foto do heap depois da operação, para somar nas médias
        soma_frag_interna += estatisticas.frag_interna;
        soma_livres += estatisticas.n_livres;
        if (estatisticas.total_livre > 0)
            soma_frag_externa += 1.0 - (double)estatisticas.maior_livre / estatisticas.total_livre;
    }

    //uma linha do CSV por execução: as médias são as somas divididas pelo número de operações
    printf("%zu,%s,%d,%zu,%zu,%zu,%.2f,%.4f,%.2f\n", min_dados, nomes_estrategias[estrategia_atual], semente,
           estatisticas.n_malloc - estatisticas.n_falhas, estatisticas.n_falhas, estatisticas.n_falhas_frag,
           soma_frag_interna / OPERACOES, soma_frag_externa / OPERACOES, soma_livres / OPERACOES);
    mem_destroy();
}

int main(void) {
    size_t valores[] = { 8, 32, 64, 128 }; //valores de min_dados testados (o padrão do alocador é 8)

    printf("min_dados,estrategia,semente,alocacoes_ok,falhas,falhas_frag,frag_interna_media,frag_externa_media,n_livres_medio\n");
    for (int v = 0; v < 4; v++) {
        min_dados = valores[v]; //troca a variável que o splitting usa
        for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) {
            for (int semente = 1; semente <= SEMENTES; semente++) {
                rodar(semente);
            }
        }
    }
    return 0;
}
