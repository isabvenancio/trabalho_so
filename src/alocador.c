#include <stdio.h>
#include <stdlib.h> /* malloc e free */
#include <string.h> /* string */
#include "alocador.h"
#include "estrategias.h"
#include "estatisticas.h"

char *heap_base = NULL; /* início da região (o driver usa para calcular o endereco) */
static size_t heap_total = 0;
static Bloco *primeiro = NULL; /* cabeça da lista implícita */
int coalescencia_ativa = 1; /* 1 = funde vizinhos livres no free; 0 = não funde */

static void splitting(Bloco* bloco, size_t tamanho);
static void coalescencia(Bloco *a, Bloco *b);

/* funções principais */
void mem_init(size_t heap_size) {
    if (heap_base != NULL)
        mem_destroy(); /* reinicialização: libera o heap anterior para não vazar */

    if (heap_size < TAM_CAB + MIN_DADOS) {
        fprintf(stderr, "ERRO: tamanho do heap insuficiente\n"); //erro padrão
        return;
    }
    /* inicializa o heap com o tamanho e o primeiro bloco livre */
    heap_base = malloc(heap_size); /* ÚNICO uso do malloc real */

    if (heap_base == NULL) {
        /* TODO: reportar erro */
        fprintf(stderr, "ERRO: nao foi possivel alocar o heap\n");//erro padrão
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

    memset(&estatisticas, 0, sizeof(estatisticas)); /* cada heap novo começa com estatísticas zeradas */
    reiniciar_next_fit();
}

void mem_destroy(void) {
    /* devolve o heap real ao sistema (único free real) */
    free(heap_base);
    heap_base = NULL;
    heap_total = 0;
    primeiro = NULL;
}

void* meu_malloc(size_t tamanho){
    /* coordena alinhamento, busca, splitting*/
    /* retorna ponteiro para o bloco alocado */
    if (heap_base == NULL) {
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED: chame mem_init antes de alocar\n");
        return NULL;
    }
    if (tamanho == 0) return NULL;

    estatisticas.n_malloc++;
    if (tamanho > heap_total) {
        estatisticas.n_falhas++;
        fprintf(stderr, "ERR_OUT_OF_MEMORY: pedido de %zu bytes maior que o heap\n", tamanho);
        return NULL;
    }

    size_t tamanho_alinhado = ALINHAR(tamanho);
    size_t exam = 0;
    Bloco *bloco = buscar(primeiro, tamanho_alinhado, &exam);

    /* TODO: registrar ’exam’ nas estatísticas */
    estatisticas.blocos_examinados += exam;

    /* TODO: se b == NULL, contabilizar falha e devolver NULL */
    if (bloco == NULL) {
        /* relatório 3.8: na falha, soma dos livres e maior bloco livre */
        size_t total_livre = 0, maior_livre = 0;
        for (Bloco *b = primeiro; b != NULL; b = b->proximo) {
            if (!b->livre) continue;
            total_livre += b->tamanho;
            if (b->tamanho > maior_livre) maior_livre = b->tamanho;
        }
        estatisticas.n_falhas++;
        if (total_livre >= tamanho_alinhado)
            estatisticas.n_falhas_frag++; /* há espaço, mas não contíguo */
        fprintf(stderr, "ERR_OUT_OF_MEMORY: sem bloco contiguo para %zu bytes "
                        "(livre total: %zu, maior livre: %zu)\n",
                tamanho, total_livre, maior_livre);
        return NULL;
    }

    splitting(bloco, tamanho_alinhado);

    /* TODO: marcar ocupado, guardar ’tamanho’ em b->pedido */
    bloco->livre = 0;
    bloco->pedido = tamanho;

    estatisticas.frag_interna += bloco->tamanho - tamanho;

    return (char *)bloco + TAM_CAB;
}

void meu_free(void* ptr){
    /*libera e faz coalescencia */
    /* verifica se o ponteiro é válido */
    if (ptr == NULL) return;
    if (heap_base == NULL) {
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED: chame mem_init antes de liberar\n");
        return;
    }
    char *p = (char *)ptr;
    if (p < heap_base + TAM_CAB || p >= heap_base + heap_total) {
        fprintf(stderr, "ERR_BAD_POINTER: ponteiro fora do heap\n");//erro padrão
        return;
    }
    Bloco *bloco = (Bloco *)(p - TAM_CAB);

    /* TODO: confirmar que b é realmente um bloco da lista */
    Bloco *atual = primeiro;
    while (atual != NULL && atual != bloco) {
        atual = atual->proximo;
    }
    if (atual == NULL) {
        fprintf(stderr, "ERR_BAD_POINTER: ponteiro nao corresponde a um bloco alocado\n"); //stderr = erro padrao
        return;
    }

    /* TODO: se b->livre, reportar double free e retornar */
    if (bloco->livre) {
        fprintf(stderr, "ERR_DOUBLE_FREE: bloco ja estava livre\n");//erro padrão
        return;
    }

    /* relatório 3.8: ao liberar, a fragmentação interna do bloco sai do total */
    estatisticas.frag_interna -= bloco->tamanho - bloco->pedido;
    /* TODO: marcar como livre */
    bloco->livre = 1;
    bloco->pedido = 0;

    /* em meu_free, depois de marcar livre: */
    if (coalescencia_ativa) {
        /* TODO: caso seguinte livre -> fundir(b, b->proximo) */
        /* TODO: caso seguinte livre -> coalescencia(b, b->proximo) */
        if (bloco->proximo != NULL && bloco->proximo->livre) {
            coalescencia(bloco, bloco->proximo);
        }
        /* TODO: caso anterior livre -> fundir(b->anterior, b) */
        /* TODO: caso anterior livre -> coalescencia(b->anterior, b) */
        if (bloco->anterior != NULL && bloco->anterior->livre) {
            coalescencia(bloco->anterior, bloco);
        }
    }
}

void mem_stats(void){
    /* exibe estatísticas da memória */
    if (heap_base == NULL) {
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED\n");
        return;
    }
    estatisticas.total_livre = 0;
    estatisticas.total_ocupado = 0;
    estatisticas.maior_livre = 0;
    estatisticas.n_livres = 0;
    estatisticas.n_ocupados = 0;
    for (Bloco *b = primeiro; b != NULL; b = b->proximo) {
        if (b->livre) {
            estatisticas.n_livres++;
            estatisticas.total_livre += b->tamanho;
            if (b->tamanho > estatisticas.maior_livre)
                estatisticas.maior_livre = b->tamanho;
        } else {
            estatisticas.n_ocupados++;
            estatisticas.total_ocupado += b->tamanho;
        }
    }
    size_t n_blocos = estatisticas.n_livres + estatisticas.n_ocupados;

    /* os if evitam divisao por zero */
    double frag_ext = 0, desp_int = 0, taxa_falha = 0, exam_medio = 0;
    if (estatisticas.total_livre > 0)
        frag_ext = 1.0 - (double)estatisticas.maior_livre / estatisticas.total_livre;
    if (estatisticas.total_ocupado > 0)
        desp_int = (double)estatisticas.frag_interna / estatisticas.total_ocupado;
    if (estatisticas.n_malloc > 0) {
        taxa_falha = 100.0 * estatisticas.n_falhas / estatisticas.n_malloc;
        exam_medio = (double)estatisticas.blocos_examinados / estatisticas.n_malloc;
    }

    printf("Heap: %zu bytes | cabecalho: %zu bytes\n", heap_total, (size_t)TAM_CAB);
    printf("Blocos: %zu ocupados, %zu livres\n", estatisticas.n_ocupados, estatisticas.n_livres);
    printf("Ocupado: %zu | Livre: %zu | Maior livre: %zu\n", estatisticas.total_ocupado, estatisticas.total_livre, estatisticas.maior_livre);
    printf("Fragmentacao interna: %zu bytes (taxa de desperdicio %.2f%%)\n", estatisticas.frag_interna, 100 * desp_int);
    printf("Fragmentacao externa: %.2f%%\n", 100 * frag_ext);
    printf("Overhead de metadados: %zu bytes (%.2f%% do heap)\n", n_blocos * TAM_CAB, 100.0 * n_blocos * TAM_CAB / heap_total);
    printf("Alocacoes: %zu | falhas: %zu (por fragmentacao: %zu) | taxa de falha: %.2f%%\n", estatisticas.n_malloc, estatisticas.n_falhas, estatisticas.n_falhas_frag, taxa_falha);
    printf("Blocos examinados por alocacao: %.2f\n", exam_medio);
}

void mem_dump(void){
    /* exibe o estado atual da memória (blocos atuais)*/
    if (heap_base == NULL) {
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED\n");
        return;
    }
    printf("%-10s %-8s %s\n", "Endereco", "Tamanho", "Estado");
    for (Bloco *bloco = primeiro; bloco != NULL; bloco = bloco->proximo) {
        size_t off = (size_t)((char *)bloco - heap_base);

        /* TODO: imprimir off, b->tamanho e LIVRE/OCUPADO */
        printf("%-10zu %-8zu %s\n", off, bloco->tamanho, bloco->livre ? "LIVRE" : "OCUPADO");
        // -10zu exibe o deslocamento do bloco em relação ao início do heap, -8zu exibe o tamanho do bloco, e %s exibe se o bloco está livre ou ocupado

        /* verificação de consistência (roteiro, passo 3) */
        Bloco *prox = bloco->proximo;
        if (prox != NULL) {
            if (prox->anterior != bloco)
                fprintf(stderr, "ALERTA: bloco %zu: proximo->anterior inconsistente\n", off);
            if ((char *)bloco + TAM_CAB + bloco->tamanho != (char *)prox)
                fprintf(stderr, "ALERTA: bloco %zu: fim nao coincide com o proximo bloco\n", off);
        }
    }
}


static void splitting(Bloco* bloco, size_t tamanho){
    if (bloco->tamanho < tamanho + TAM_CAB + MIN_DADOS)
        return; /* sobra pequena: não divide */
    Bloco *novo = (Bloco *)((char *)bloco + TAM_CAB + tamanho);
    novo->tamanho = /* TODO */ bloco->tamanho - tamanho - TAM_CAB;
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
    a->proximo = b->proximo;
    if (b->proximo != NULL)
        b->proximo->anterior = a;

    reiniciar_next_fit(); /* o next-fit pode estar guardando o bloco b, que deixou de existir */

    /* em meu_free, depois de marcar livre: */
    // if (coalescencia_ativa) {
    /* TODO: caso seguinte livre -> fundir(b, b->proximo) */
    /* TODO: caso anterior livre -> fundir(b->anterior, b) */
    // }
}