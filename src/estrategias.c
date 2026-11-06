#include "estrategias.h"

const char *nomes_estrategias[] = { "first_fit", "next_fit", "best_fit", "worst_fit" }; //nomes_estrategias[BEST_FIT] é "best_fit"
Estrategia estrategia_atual = FIRST_FIT; //estratégia padrão

/* chama a busca da estratégia escolhida */
Bloco *buscar(Bloco *inicio, size_t tamanho, size_t *examinados){
    switch (estrategia_atual) {
        case NEXT_FIT:
            return buscar_next_fit(inicio, tamanho, examinados);
        case BEST_FIT:
            return buscar_best_fit(inicio, tamanho, examinados);
        case WORST_FIT:
            return buscar_worst_fit(inicio, tamanho, examinados);
        //case FIRST_FIT:
        default: //FIRST_FIT (e qualquer outro valor) usa o first-fit
            return buscar_first_fit(inicio, tamanho, examinados);
    }
}


Bloco *buscar_first_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo){ //percorre do primeiro ao último bloco
        (*examinados)++; //conta mais um bloco olhado

        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
        if (bloco->livre && bloco->tamanho >= tamanho){
            return bloco; // se o bloco é livre e tem tamanho suficiente, retorna o bloco
        }
    }
    return NULL; //nenhum bloco serve
}

static Bloco *ultimo_next_fit = NULL; /* onde a última busca next-fit parou (NULL = começa do início) */

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
    Bloco *partida = (ultimo_next_fit != NULL) ? ultimo_next_fit : inicio; //começa de onde parou, ou do início na primeira vez
    Bloco *bloco = partida;

    /* percorre a lista de forma circular, dando no máximo uma volta completa */
    do {
        (*examinados)++; //conta mais um bloco olhado

        if (bloco->livre && bloco->tamanho >= tamanho){
            ultimo_next_fit = bloco; // a próxima busca recomeça deste bloco
            return bloco;
        }
        bloco = (bloco->proximo != NULL) ? bloco->proximo : inicio; //chegou no fim do heap: volta para o início
    } while (bloco != partida); //voltou para onde começou: deu a volta inteira

    return NULL; //nenhum bloco serve
}

Bloco* buscar_best_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    Bloco *melhor = NULL; //menor bloco que serve, até agora
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) { //precisa olhar todos os blocos
        (*examinados)++;
        /* TODO: se b é candidato e é melhor que ’melhor’, atualize */
        if (bloco->livre && bloco->tamanho >= tamanho) {
            if (melhor == NULL || bloco->tamanho < melhor->tamanho) { //'<' e não '<=': no empate fica o primeiro encontrado
                melhor = bloco; //se o bloco é livre e tem tamanho suficiente, e é melhor que o melhor bloco encontrado até agora, atualiza o melhor bloco
            }
        }
    }
    return melhor;
}

Bloco* buscar_worst_fit(Bloco *inicio, size_t tamanho, size_t *examinados){
    Bloco *pior = NULL; //maior bloco que serve, até agora
    for (Bloco *bloco = inicio; bloco != NULL; bloco = bloco->proximo) { //precisa olhar todos os blocos
        (*examinados)++;
        /* TODO: se b é candidato e é pior que ’pior’, atualize */
        if (bloco->livre && bloco->tamanho >= tamanho) {
            if (pior == NULL || bloco->tamanho > pior->tamanho) { //'>' e não '>=': no empate fica o primeiro encontrado
                pior = bloco; //se o bloco é livre e tem tamanho suficiente, e é pior que o pior bloco encontrado até agora, atualiza o pior bloco
            }
        }
    }
    return pior;
}
