/* Workload 3: padrão adversarial.
 * Aloca N blocos pequenos, libera os de índice par e pede um bloco do tamanho da soma
 * dos buracos: cabe na soma, mas em nenhum buraco sozinho.
 *
 * Experimento de coalescência: depois, libera também os blocos ímpares e repete o pedido.
 * Com coalescência, tudo vira um único bloco e o pedido dá certo. Sem ela, o heap
 * continua dividido em blocos pequenos e o pedido falha, mesmo com o heap todo livre. */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define N_BLOCOS 20
#define TAMANHO 64

int main(void) {
    size_t heap = N_BLOCOS * (TAM_CAB + TAMANHO); /* tamanho exato: não sobra nada no fim do heap */
    size_t pedido = TAMANHO * (N_BLOCOS / 2);     /* a soma dos buracos */

    Estrategia estrategias[] = { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT };

    /* 1 = coalescência ligada, 0 = desligada */
    for (int c = 1; c >= 0; c--) {
        coalescencia_ativa = c;
        for (int i = 0; i < 4; i++) { //uma vez para cada estratégia
            estrategia_atual = estrategias[i];
            void *blocos[N_BLOCOS]; //ponteiros dos blocos pequenos
            int operacao = 0; //número da operação, para a coluna 'operacao' do CSV

            mem_init(heap);

            for (int i = 0; i < N_BLOCOS; i++) { //1. enche o heap com os blocos pequenos
                operacao++;
                blocos[i] = alocar_e_registrar("workload3", operacao, heap, TAMANHO);
            }

            for (int i = 0; i < N_BLOCOS; i += 2) { /* 2. libera os de índice par: 0, 2, 4... abrindo buracos separados */
                operacao++;
                liberar_e_registrar("workload3", operacao, heap, blocos[i]);
            }

            operacao++;
            alocar_e_registrar("workload3", operacao, heap, pedido); /* 3. pede a soma dos buracos: deve falhar nos dois modos */
            mem_dump_csv("workload3_falha"); /* os buracos, no momento da falha */

            for (int i = 1; i < N_BLOCOS; i += 2) { /* 4. libera os de índice ímpar: 1, 3, 5... agora o heap está todo livre */
                operacao++;
                liberar_e_registrar("workload3", operacao, heap, blocos[i]);
            }

            operacao++;
            alocar_e_registrar("workload3", operacao, heap, pedido); /* 5. repete o pedido: só dá certo com coalescência */

            mem_dump_csv("workload3"); //como o heap ficou no fim
            mem_destroy();
        }
    }
    return 0;
}
