#include <stdio.h>
#include "alocador.h"
#ifndef ESTRATEGIAS_H
#define ESTRATEGIAS_H

typedef enum { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT } Estrategia;

Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados);
Bloco *buscar_next_fit(Bloco *inicio, size_t tamanho, size_t *examinados);
Bloco *buscar_best_fit (Bloco *inicio, size_t tamanho, size_t *examinados);
Bloco *buscar_worst_fit(Bloco *inicio, size_t tamanho, size_t *examinados);
typedef Bloco *(*FuncaoBusca)(Bloco *, size_t, size_t *);


void definir_estrategia(Estrategia estrategia);
void reiniciar_next_fit(void); /* chamar em mem_init e quando o bloco guardado for fundido */
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados);

#endif // ESTRATEGIAS_H

