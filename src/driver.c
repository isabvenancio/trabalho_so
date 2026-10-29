#include <stdio.h>
#include <string.h>
#include "alocador.h"
#include "estrategias.h"

/* Driver de testes: le comandos de um arquivo, um por linha.
 *   ALLOC <tamanho>   FREE <id>   DUMP   STATS
 *   HEAP <tamanho>    ESTRATEGIA first|best|worst|next   COALESCENCIA 0|1
 * Linhas vazias ou comecando com # sao ignoradas.
 * O id de cada bloco segue a ordem das alocacoes que deram certo (1, 2, 3, ...).
 * Se o arquivo nao tiver HEAP, o heap tem HEAP_PADRAO bytes. */

#define MAX_BLOCOS 1024
#define HEAP_PADRAO 1000

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <arquivo de comandos>\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }

    void *ponteiros[MAX_BLOCOS + 1]; /* ponteiros[id]; a posicao 0 nao e usada */
    int n_blocos = 0;
    char linha[256];
    char comando[32];
    char texto[32];
    long valor;

    mem_init(HEAP_PADRAO);

    while (fgets(linha, sizeof(linha), f) != NULL) {
        if (linha[0] == '#' || sscanf(linha, "%31s", comando) != 1)
            continue; /* comentario ou linha vazia */

        if (strcmp(comando, "ALLOC") == 0 && sscanf(linha, "%*s %ld", &valor) == 1) {
            if (valor <= 0) {
                printf("ERRO: tamanho invalido (%ld)\n", valor);
            } else if (n_blocos == MAX_BLOCOS) {
                printf("ERRO: o driver so guarda %d blocos\n", MAX_BLOCOS);
            } else {
                void *p = meu_malloc((size_t)valor);
                if (p == NULL) {
                    printf("ERRO: ALLOC %ld falhou\n", valor); /* o motivo sai na mensagem do alocador */
                } else {
                    n_blocos++;
                    ponteiros[n_blocos] = p;
                    Bloco *b = (Bloco *)((char *)p - TAM_CAB);
                    printf("Bloco %d alocado em endereco %zu, tamanho %zu\n",
                           n_blocos, (size_t)((char *)b - heap_base), b->tamanho);
                }
            }
        } else if (strcmp(comando, "FREE") == 0 && sscanf(linha, "%*s %ld", &valor) == 1) {
            if (valor < 1 || valor > n_blocos) {
                printf("ERRO: tentativa de liberar bloco inexistente (id %ld)\n", valor);
            } else {
                /* o ponteiro nao e apagado depois do free: assim um FREE repetido
                   chega no meu_free e o alocador detecta o double free */
                meu_free(ponteiros[valor]);
                if (ultimo_status == ALOC_OK)
                    printf("Bloco %ld liberado\n", valor);
            }
        } else if (strcmp(comando, "DUMP") == 0) {
            mem_dump();
        } else if (strcmp(comando, "STATS") == 0) {
            mem_stats();
        } else if (strcmp(comando, "HEAP") == 0 && sscanf(linha, "%*s %ld", &valor) == 1) {
            if (valor <= 0) {
                printf("ERRO: tamanho de heap invalido (%ld)\n", valor);
            } else {
                mem_init((size_t)valor);
                n_blocos = 0; /* os blocos do heap antigo deixaram de existir */
            }
        } else if (strcmp(comando, "ESTRATEGIA") == 0 && sscanf(linha, "%*s %31s", texto) == 1) {
            if (strcmp(texto, "first") == 0)      definir_estrategia(FIRST_FIT);
            else if (strcmp(texto, "best") == 0)  definir_estrategia(BEST_FIT);
            else if (strcmp(texto, "worst") == 0) definir_estrategia(WORST_FIT);
            else if (strcmp(texto, "next") == 0)  definir_estrategia(NEXT_FIT);
            else printf("ERRO: estrategia desconhecida (%s)\n", texto);
        } else if (strcmp(comando, "COALESCENCIA") == 0 && sscanf(linha, "%*s %ld", &valor) == 1) {
            coalescencia_ativa = (valor != 0);
        } else {
            printf("ERRO: comando invalido: %s", linha);
        }
        fflush(stdout); /* mantem a ordem das mensagens de stdout e stderr no make exemplos */
    }

    fclose(f);
    mem_destroy();
    return 0;
}
