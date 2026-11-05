#include "estrategias.h"

const char *nomes_estrategias[] = { "first_fit", "next_fit", "best_fit", "worst_fit" };
Estrategia estrategia_atual = FIRST_FIT;

/* chama a busca da estratégia escolhida */
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados){
    switch (estrategia_atual) {
        case NEXT_FIT:  return buscar_next_fit(inicio, tamanho, examinados);
        case BEST_FIT:  return buscar_best_fit(inicio, tamanho, examinados);
        case WORST_FIT: return buscar_worst_fit(inicio, tamanho, examinados);
        case FIRST_FIT:
        default:        return buscar_first_fit(inicio, tamanho, examinados);
    }
}


Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo){
        (*examinados)++;

        if (bloco->livre && bloco->tamanho >= tamanho){
            return bloco; // se o bloco é livre e tem tamanho suficiente, retorna o bloco
        }
    }
    return NULL;
}

static Bloco *ultimo_next_fit = NULL; /* onde a última busca next-fit parou */

void reiniciar_next_fit(void){
    ultimo_next_fit = NULL;
}

/* o bloco b deixou de existir (virou parte de a): se o next-fit estava guardando b, passa a guardar a */
void next_fit_bloco_fundido(Bloco *a, Bloco *b){
    if (ultimo_next_fit == b){
        ultimo_next_fit = a;
    }
}

Bloco *buscar_next_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    if (inicio == NULL){
        return NULL;
    }
    Bloco *partida = (ultimo_next_fit != NULL) ? ultimo_next_fit : inicio;
    Bloco *bloco = partida;

    /* percorre a lista de forma circular, dando no máximo uma volta completa */
    do {
        (*examinados)++;

        if (bloco->livre && bloco->tamanho >= tamanho){
            ultimo_next_fit = bloco; // a próxima busca recomeça deste bloco
            return bloco;
        }
        bloco = (bloco->proximo != NULL) ? bloco->proximo : inicio;
    } while (bloco != partida);

    return NULL;
}

Bloco* buscar_best_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    Bloco *melhor = NULL;
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) {
        (*examinados)++;
        if (bloco->livre && bloco->tamanho >= tamanho) {
            if (melhor == NULL || bloco->tamanho < melhor->tamanho) {
                melhor = bloco; //se o bloco é livre e tem tamanho suficiente, e é melhor que o melhor bloco encontrado até agora, atualiza o melhor bloco
            }
        }
    }
    return melhor;
}

Bloco* buscar_worst_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    Bloco *pior = NULL;
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) {
        (*examinados)++;
        if (bloco->livre && bloco->tamanho >= tamanho) {
            if (pior == NULL || bloco->tamanho > pior->tamanho) {
                pior = bloco; //se o bloco é livre e tem tamanho suficiente, e é pior que o pior bloco encontrado até agora, atualiza o pior bloco
            }
        }
    }
    return pior;
}

