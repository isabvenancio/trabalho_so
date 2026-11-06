/* Workload 1: alocações sequenciais de tamanhos variados, sem liberar nada, até o heap esgotar.
 * Mede quantas alocações couberam, a fragmentação interna e o overhead de metadados.
 * Como nada é liberado, só existe um bloco livre (o do fim): as estratégias escolhem
 * sempre o mesmo bloco e só muda quantos blocos cada uma examina. */
#include <stdlib.h> /* rand e srand */
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define HEAP 4096
#define SEMENTE 42

int main(void) {
    for (estrategia_atual = FIRST_FIT; estrategia_atual <= WORST_FIT; estrategia_atual++) { //roda uma vez para cada estratégia
        srand(SEMENTE); /* mesma semente: todas as estratégias recebem os mesmos pedidos */
        mem_init(HEAP);

        int operacao = 0;
        void *p;
        do {
            operacao++;
            p = alocar_e_registrar("workload1", operacao, HEAP, 10 + rand() % 141); /* pedido de 10 a 150 bytes; grava uma linha no CSV */
        } while (p != NULL); /* para no primeiro pedido que falhar */

        mem_dump_csv("workload1"); //como o heap ficou no fim (gráfico de barra do heap)
        mem_destroy();
    }
    return 0;
}
