#include <stdio.h>
#include <stdlib.h> /* malloc e free */
#include <string.h> /* string */
#include "alocador.h"
#include "estrategias.h"
#include "estatisticas.h"

/*
git status
git add .
git commit -m "descricao da alteracao"
git push
*/

static char *heap_base = NULL; /* início da região */
static size_t heap_total = 0;
static Bloco *primeiro = NULL; /* cabeça da lista implícita */

/* funções principais */
void mem_init(size_t heap_size) {
    if (heap_size < TAM_CAB + MIN_DADOS) {
        fprintf(stderr, "ERRO: tamanho do heap insuficiente\n");
        return;
    }
    /* inicializa o heap com o tamanho e o primeiro bloco livre */
    heap_base = malloc(heap_size); /* ÚNICO uso do malloc real */

    if (heap_base == NULL) {
        /* TODO: reportar erro */ 
        fprintf(stderr, "ERRO: nao foi possivel alocar o heap\n");
        heap_total = 0;
        primeiro = NULL;
        return;
        }
    heap_total = heap_size;
    primeiro = (Bloco *) heap_base;
    /* TODO: preencher tamanho, livre, proximo e anterior */

    primeiro->tamanho = heap_size - TAM_CAB;
    primeiro->pedido = 0;
    primeiro->livre = 1;
    primeiro->proximo = NULL;
    primeiro->anterior = NULL;
}

void* meu_malloc(size_t tamanho){
    /* coordena alinhamento, busca, splitting*/
    /* retorna ponteiro para o bloco alocado */
    if (tamanho == 0) return NULL;
    size_t tamanho_alinhado = ALINHAR(tamanho);
    size_t exam = 0;
    Bloco *bloco = buscar(primeiro, tamanho_alinhado, &exam);

    /* TODO: registrar ’exam’ nas estatísticas */
    estatisticas.n_malloc++;
    estatisticas.blocos_examinados += exam;

    /* TODO: registrar ’exam’ nas estatísticas */
    estatisticas.n_malloc++;
    estatisticas.blocos_examinados += exam;

    /* TODO: se b == NULL, contabilizar falha e devolver NULL */
    if (bloco == NULL) {
        estatisticas.n_falhas++;
        return NULL;
    }

    splitting(bloco, tamanho_alinhado);
    
    bloco->livre = 0;
    bloco->pedido = tamanho;

    /* TODO: marcar ocupado, guardar ’tamanho’ em b->pedido */
    bloco->livre = 0;
    bloco->pedido = tamanho;

    return (char *)bloco + TAM_CAB;
}

void meu_free(void* ptr){
    /*libera e faz coalescencia */
    /* verifica se o ponteiro é válido */
    if (ptr == NULL) return;
    char *p = (char *)ptr;
    if (p < heap_base + TAM_CAB || p >= heap_base + heap_total) {
        fprintf(stderr, "ERRO: ponteiro fora do heap\n");
        return;
    }
    Bloco *bloco = (Bloco *)(p - TAM_CAB);
    /* TODO: confirmar que b é realmente um bloco da lista */
    /* TODO: se b->livre, reportar double free e retornar */
    /* TODO: marcar como livre */
}

void mem_stats(void){
    /* exibe estatísticas da memória */
}

void mem_dump(void){
    /* exibe o estado atual da memória (blocos atuais)*/
    printf("%-10s %-8s %s\n", "Endereco", "Tamanho", "Estado");
    for (Bloco *bloco = primeiro; bloco != NULL; bloco = bloco->proximo) {
        size_t off = (size_t)((char *)bloco - heap_base);
        
        /* TODO: imprimir off, b->tamanho e LIVRE/OCUPADO */
        printf("%-10zu %-8zu %s\n", off, bloco->tamanho, bloco->livre ? "LIVRE" : "OCUPADO");
        // -10zu exibe o deslocamento do bloco em relação ao início do heap, -8zu exibe o tamanho do bloco, e %s exibe se o bloco está livre ou ocupado
    }
}


static void splitting(Bloco* bloco, size_t tamanho){
    if (bloco->tamanho < tamanho + TAM_CAB + MIN_DADOS)
        return; /* sobra pequena: não divide */
    Bloco *novo = (Bloco *)((char *)bloco + TAM_CAB + tamanho);
    novo->tamanho = bloco->tamanho - tamanho - TAM_CAB;/* TODO */
    novo->pedido = 0;
    novo->livre = 1;

    /* TODO: encadear ’novo’ entre bloco e bloco->proximo (4 ponteiros!) */
    novo->proximo = bloco->proximo;
    novo->anterior = bloco;

    if (bloco->proximo != NULL)
        bloco->proximo->anterior = novo;

    bloco->proximo = novo;

    bloco->tamanho = tamanho;
}

static void coalescencia(Bloco *a, Bloco *b) { //próximo passo
    /* pré-condição: a e b livres e b == a->proximo */
    a->tamanho += TAM_CAB + b->tamanho;
    /* TODO: religar a->proximo e o ’anterior’ do seguinte de b */
    
    /* em meu_free, depois de marcar livre: */
    // if (coalescencia_ativa) {
    //     p
    /* TODO: caso seguinte livre -> fundir(b, b->proximo) */
    /* TODO: caso anterior livre -> fundir(b->anterior, b) */
}