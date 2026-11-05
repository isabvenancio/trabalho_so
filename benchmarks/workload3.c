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

    /* 1 = coalescência ligada, 0 = desligada */
    for (coalescencia_ativa = 1; coalescencia_ativa >= 0; coalescencia_ativa--) {
        for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) {
            void *blocos[N_BLOCOS];
            int operacao = 0;

            mem_init(heap);

            for (int i = 0; i < N_BLOCOS; i++) {
                operacao++;
                blocos[i] = alocar_e_registrar("workload3", operacao, heap, TAMANHO);
            }

            for (int i = 0; i < N_BLOCOS; i += 2) { /* índices pares: 0, 2, 4... */
                operacao++;
                liberar_e_registrar("workload3", operacao, heap, blocos[i]);
            }

            operacao++;
            alocar_e_registrar("workload3", operacao, heap, pedido); /* deve falhar nos dois modos */
            mem_dump_csv("workload3_falha"); /* os buracos, no momento da falha */

            for (int i = 1; i < N_BLOCOS; i += 2) { /* índices ímpares: 1, 3, 5... */
                operacao++;
                liberar_e_registrar("workload3", operacao, heap, blocos[i]);
            }

            operacao++;
            alocar_e_registrar("workload3", operacao, heap, pedido); /* só dá certo com coalescência */

            mem_dump_csv("workload3");
            mem_destroy();
        }
    }
    return 0;
}
