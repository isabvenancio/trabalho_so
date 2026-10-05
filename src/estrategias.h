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
static FuncaoBusca buscar; /* definida em mem_init conforme a estratégia */

void definir_estrategia(Estrategia estrategia);
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados);

#endif // ESTRATEGIAS_H

