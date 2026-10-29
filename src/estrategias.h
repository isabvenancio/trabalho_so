#ifndef ESTRATEGIAS_H
#define ESTRATEGIAS_H

#include "alocador.h"

typedef enum { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT } Estrategia;

//percorrem a lista, somam 1 em examinados a cada bloco olhado (é a métrica de custo) e devolvem um bloco livre com tamanho >= pedido e nenhuma altera a lista.
Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados);//devolve o primeiro que serve e para ali.
Bloco *buscar_next_fit(Bloco *inicio, size_t tamanho, size_t *examinados); //começam de onde a última busca parou (ultimo_next_fit) e dão uma volta circular na lista
Bloco *buscar_best_fit (Bloco *inicio, size_t tamanho, size_t *examinados);//olha todos e guarda o menor que serve. Em caso de empate fica o primeiro encontrado
Bloco *buscar_worst_fit(Bloco *inicio, size_t tamanho, size_t *examinados);//olha todos e guarda o maior.

extern Estrategia estrategia_atual; /* estratégia usada por buscar() (padrão: FIRST_FIT) */

void definir_estrategia(Estrategia estrategia);//só faz estrategia_atual = e.
void reiniciar_next_fit(void); /* chamar em mem_init e quando o bloco guardado for fundido */ //começam de onde a última busca parou (ultimo_next_fit) e dão uma volta circular na lista
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados);//o switch que chama a função da estratégia atual.

#endif // ESTRATEGIAS_H

