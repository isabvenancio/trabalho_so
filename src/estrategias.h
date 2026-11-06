#ifndef ESTRATEGIAS_H 
#define ESTRATEGIAS_H

#include "alocador.h"

typedef enum { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT } Estrategia; //valem 0, 1, 2 e 3, nessa ordem

//percorrem a lista, somam 1 em examinados a cada bloco olhado (é a métrica de custo) e devolvem um bloco livre com tamanho >= pedido e nenhuma altera a lista.
Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados);//devolve o primeiro que serve e para ali.
Bloco *buscar_next_fit(Bloco *inicio, size_t tamanho, size_t *examinados); //começa de onde a última busca parou (ultimo_next_fit) e dá uma volta circular na lista
Bloco *buscar_best_fit (Bloco *inicio, size_t tamanho, size_t *examinados);//olha todos e guarda o menor que serve. Em caso de empate fica o primeiro encontrado
Bloco *buscar_worst_fit(Bloco *inicio, size_t tamanho, size_t *examinados);//olha todos e guarda o maior.

extern const char *nomes_estrategias[]; /* nomes usados nos CSVs, na mesma ordem do enum */
extern Estrategia estrategia_atual; /* estratégia usada por buscar(); para trocar é só atribuir (padrão: FIRST_FIT) */

void reiniciar_next_fit(void); /* chamar em mem_init: a próxima busca começa do início do heap */
void next_fit_bloco_fundido(Bloco *a, Bloco *b); /* chamar quando b for fundido em a */
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados);//o switch que chama a função da estratégia atual.

#endif