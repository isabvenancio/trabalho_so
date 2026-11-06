/* Driver de testes: le comandos de um arquivo, um por linha.
 *   ALLOC <tamanho>   FREE <id>   DUMP   STATS
 *   HEAP <tamanho>    ESTRATEGIA first|next|best|worst   COALESCENCIA 0|1
 * Linhas vazias ou comecando com # sao ignoradas.
 * O id de cada bloco segue a ordem das alocacoes que deram certo (1, 2, 3, ...).
 * Se o arquivo nao tiver HEAP, o heap tem HEAP_PADRAO bytes.
 * Uso: ./driver exemplos/entrada1.txt (o make exemplos compara a saida com exemplos/saida1.txt) */
#include <stdio.h>
#include <string.h> /* strcmp */
#include "alocador.h"
#include "estrategias.h"

#define MAX_BLOCOS 1024 //quantos ids o driver consegue guardar
#define HEAP_PADRAO 1000

int main(int argc, char *argv[]) {
    if (argc != 2) { //precisa receber o nome do arquivo de comandos
        fprintf(stderr, "uso: %s <arquivo de comandos>\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]); //mostra o motivo, ex.: arquivo não existe
        return 1;
    }

    void *ponteiros[MAX_BLOCOS + 1]; /* ponteiros[id]; a posicao 0 nao e usada */
    int n_blocos = 0; //quantos blocos já foram alocados (é o último id)
    char linha[256];  //a linha inteira lida do arquivo
    char comando[32]; //a primeira palavra da linha: ALLOC, FREE...
    char texto[32];   //o argumento quando ele é texto (ESTRATEGIA best)
    long valor;       //o argumento quando ele é número (ALLOC 100)

    mem_init(HEAP_PADRAO);

    while (fgets(linha, sizeof(linha), f) != NULL) { //lê uma linha por vez até o fim do arquivo
        if (linha[0] == '#' || sscanf(linha, "%31s", comando) != 1)
            continue; /* comentario ou linha vazia */

        //em cada comando, o sscanf com %*s pula a primeira palavra e lê o argumento
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
                    ponteiros[n_blocos] = p; //guarda o ponteiro com o próximo id
                    Bloco *b = (Bloco *)((char *)p - TAM_CAB); //volta para o cabeçalho para mostrar o tamanho real
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
                if (ultimo_status == ALOC_OK) //só diz que liberou se o alocador não acusou erro
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
                mem_init((size_t)valor); //o mem_init libera o heap antigo antes de criar o novo
                n_blocos = 0; /* os blocos do heap antigo deixaram de existir */
            }
        } else if (strcmp(comando, "ESTRATEGIA") == 0 && sscanf(linha, "%*s %31s", texto) == 1) {
            if (strcmp(texto, "first") == 0)      estrategia_atual = FIRST_FIT;
            else if (strcmp(texto, "next") == 0)  estrategia_atual = NEXT_FIT;
            else if (strcmp(texto, "best") == 0)  estrategia_atual = BEST_FIT;
            else if (strcmp(texto, "worst") == 0) estrategia_atual = WORST_FIT;
            else printf("ERRO: estrategia desconhecida (%s)\n", texto);
        } else if (strcmp(comando, "COALESCENCIA") == 0 && sscanf(linha, "%*s %ld", &valor) == 1) {
            coalescencia_ativa = (valor != 0); //qualquer valor diferente de 0 liga
        } else {
            printf("ERRO: comando invalido: %s", linha); //comando desconhecido ou sem o argumento
        }
        fflush(stdout); /* mantem a ordem das mensagens de stdout e stderr no make exemplos */
    }

    fclose(f);
    mem_destroy();
    return 0;
}
