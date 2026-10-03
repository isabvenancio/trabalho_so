#include "estrategias.h"

Bloco *buscar_first_fit(Bloco *inicio, size_t tam, size_t *examinados){
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo){
        (*examinados)++;

        if (bloco->livre && bloco->tamanho >= tam){
            return bloco;
        }
    }

    return NULL;
}

Bloco *buscar_next_fit(Bloco *inicio, size_t tam, size_t *examinados){

}

Bloco* buscar_best_fit(Bloco *inicio, size_t tam, size_t *examinados){
    Bloco *melhor = NULL;
    for (Bloco *b = inicio; b != NULL; b = b->proximo) {
        (*examinados)++;
        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
    //}
    return melhor;
    }
}

Bloco* buscar_worst_fit(Bloco *inicio, size_t tam, size_t *examinados){
    Bloco *pior = NULL;
    for (Bloco *b = inicio; b != NULL; b = b->proximo) {
        (*examinados)++;
        /* TODO: se b é candidato e é pior que ’pior’, atualize */
    //}
    return pior;
    }
}