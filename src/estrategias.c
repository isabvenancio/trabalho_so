#include "estrategias.h"

static FuncaoBusca funcao_busca = buscar_first_fit;

void definir_estrategia(Estrategia estrategia){
    if (estrategia == FIRST_FIT){
        funcao_busca = buscar_first_fit;
    }
    else if (estrategia == NEXT_FIT){
        funcao_busca = buscar_next_fit;
    }
    else if (estrategia == BEST_FIT){
        funcao_busca = buscar_best_fit;
    }
    else if (estrategia == WORST_FIT){
        funcao_busca = buscar_worst_fit;
    }
}

Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados){
    return funcao_busca(inicio, tamanho, examinados);
}


Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo){
        (*examinados)++;

        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
        if (bloco->livre && bloco->tamanho >= tamanho){
            return bloco; // se o bloco é livre e tem tamanho suficiente, retorna o bloco
        }
    }
    return NULL;
}

Bloco *buscar_next_fit(Bloco *inicio, size_t tamanho, size_t *examinados){

}

Bloco* buscar_best_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    Bloco *melhor = NULL;
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) {
        (*examinados)++;
        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
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
        /* TODO: se b é candidato e é pior que ’pior’, atualize */
        if (bloco->livre && bloco->tamanho >= tamanho) {
            if (pior == NULL || bloco->tamanho > pior->tamanho) {
                pior = bloco; //se o bloco é livre e tem tamanho suficiente, e é pior que o pior bloco encontrado até agora, atualiza o pior bloco
            }
        }
    }
    return pior;
}

