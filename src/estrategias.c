#include "estrategias.h"

Bloco *buscar_first_fit(Bloco *inicio, size_t tam, size_t *examinados){
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo){
        (*examinados)++;

        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
        if (bloco->livre && bloco->tamanho >= tam){
            return bloco; // se o bloco é livre e tem tamanho suficiente, retorna o bloco
        }
    }
    return NULL;
}

Bloco *buscar_next_fit(Bloco *inicio, size_t tam, size_t *examinados){

}

Bloco* buscar_best_fit(Bloco *inicio, size_t tam, size_t *examinados){
    Bloco *melhor = NULL;
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) {
        (*examinados)++;
        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
        if (bloco->livre && bloco->tamanho >= tam) {
            if (melhor == NULL || bloco->tamanho < melhor->tamanho) {
                melhor = bloco; //se o bloco é livre e tem tamanho suficiente, e é melhor que o melhor bloco encontrado até agora, atualiza o melhor bloco
            }
        }
    }
    return melhor;
}

Bloco* buscar_worst_fit(Bloco *inicio, size_t tam, size_t *examinados){
    Bloco *pior = NULL;
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) {
        (*examinados)++;
        /* TODO: se b é candidato e é pior que ’pior’, atualize */
        if (bloco->livre && bloco->tamanho >= tam) {
            if (pior == NULL || bloco->tamanho > pior->tamanho) {
                pior = bloco; //se o bloco é livre e tem tamanho suficiente, e é pior que o pior bloco encontrado até agora, atualiza o pior bloco
            }
        }
    }
    return pior;
}