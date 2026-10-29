#include <stdio.h>
#include <stdlib.h> /* malloc e free */
#include <string.h> /* memset */
#include "alocador.h"
#include "estrategias.h"
#include "estatisticas.h"

char *heap_base = NULL; /* início da região (o driver usa para calcular o endereco) */
static size_t heap_total = 0;
static Bloco *primeiro = NULL; /* cabeça da lista implícita */
int coalescencia_ativa = 1; /* 1 = funde vizinhos livres no free; 0 = não funde */
AlocStatus ultimo_status = ALOC_OK; /* deu certo ou qual erro aconteceu na última chamada */

static void splitting(Bloco* bloco, size_t tamanho);
static void coalescencia(Bloco *a, Bloco *b);
static void contar_blocos(void);

// heap: [cab|dados....][cab|dados..][cab|dados...........]
//        ^ primeiro      ^ proximo    ^ proximo -> NULL
// TAM_CAB é 40 bytes e MIN_DADOS é 8 bytes

/* funções principais */
void mem_init(size_t heap_size) { //prepara o heap
    if (heap_base != NULL) //inicio do heap
        mem_destroy(); /* reinicialização: libera o heap anterior para não vazar */ //Se já existia um heap, libera o antigo com mem_destroy.

    if (heap_size < TAM_CAB + MIN_DADOS) {
        fprintf(stderr, "ERRO: tamanho do heap insuficiente\n"); //erro padrão, recusa um heap menor q um cabeçalho mais 8 bytes
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

    /* TODO: preencher tamanho, livre, proximo e anterior */ //primeiro é o primeiro bloco da lista
    primeiro->tamanho = heap_size - TAM_CAB; //Escreve um cabeçalho no início: um único bloco livre de heap_size - 40 bytes.
    primeiro->pedido = 0;
    primeiro->livre = 1;
    primeiro->proximo = NULL;
    primeiro->anterior = NULL;

    memset(&estatisticas, 0, sizeof(estatisticas)); /* cada heap novo começa com estatísticas zeradas  e o nextift tbm zera*/
    reiniciar_next_fit();
}

void mem_destroy(void) {
    /* devolve o heap real ao sistema (único free real) e zera as variáveis */
    free(heap_base);
    heap_base = NULL;
    heap_total = 0;
    primeiro = NULL;
}

void* meu_malloc(size_t tamanho){//aloca
    /* coordena alinhamento, busca, splitting*/
    /* retorna ponteiro para o bloco alocado */
    if (heap_base == NULL) { //Erro se o heap não foi inicializado. Devolve NULL se tamanho == 0.
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED: chame mem_init antes de alocar\n");
        ultimo_status = ERR_HEAP_NOT_INITIALIZED;
        return NULL;
    }
    if (tamanho == 0) {
        ultimo_status = ALOC_OK; /* pedir 0 bytes não é erro: só devolve NULL */
        return NULL; 
    }

    estatisticas.n_malloc++;
    if (tamanho > heap_total) {
        estatisticas.n_falhas++; //Conta o pedido (n_malloc++). Se o pedido for maior que o heap inteiro, é falha imediata.
        fprintf(stderr, "ERR_OUT_OF_MEMORY: pedido de %zu bytes maior que o heap\n", tamanho);
        ultimo_status = ERR_OUT_OF_MEMORY;
        return NULL;
    }

    size_t tamanho_alinhado = ALINHAR(tamanho); //Alinha o tamanho e chama buscar(), que usa a estratégia ativa e devolve um bloco livre que sirva.
    size_t exam = 0;
    Bloco *bloco = buscar(primeiro, tamanho_alinhado, &exam);

    /* TODO: registrar ’exam’ nas estatísticas */
    estatisticas.blocos_examinados += exam;

    /* TODO: se b == NULL, contabilizar falha e devolver NULL */
    if (bloco == NULL) { //Se não achou: chama contar_blocos(), registra a falha e decide se foi por fragmentação (havia espaço livre somado suficiente, mas nenhum bloco inteiro grande o bastante).
        /* relatório 3.8: na falha, soma dos livres e maior bloco livre */
        contar_blocos();
        estatisticas.n_falhas++;
        if (estatisticas.total_livre >= tamanho_alinhado){
            estatisticas.n_falhas_frag++;/* há espaço, mas não contíguo */
            ultimo_status = ERR_FRAGMENTATION;
            fprintf(stderr, "ERR_FRAGMENTATION: ");
        } else {
            ultimo_status = ERR_OUT_OF_MEMORY;
            fprintf(stderr, "ERR_OUT_OF_MEMORY: ");
        }
        fprintf(stderr,"sem bloco contiguo para %zu bytes (livre total: %zu, maior livre: %zu)\n", tamanho, estatisticas.total_livre, estatisticas.maior_livre);
        return NULL;
    }

    //Se achou: chama splitting para cortar a sobra, marca como ocupado, guarda o pedido e soma a fragmentação interna (tamanho do bloco - pedido).
    splitting(bloco, tamanho_alinhado);

    /* TODO: marcar ocupado, guardar ’tamanho’ em b->pedido */
    bloco->livre = 0;
    bloco->pedido = tamanho;

    estatisticas.frag_interna += bloco->tamanho - tamanho;

    //Devolve o endereço dos dados: bloco + TAM_CAB, ou seja, pula o cabeçalho.
    ultimo_status = ALOC_OK;
    return (char *)bloco + TAM_CAB;
}

void meu_free(void* ptr){//libera
    /*libera e faz coalescencia */
    /* verifica se o ponteiro é válido */
    if (ptr == NULL) { 
        ultimo_status = ALOC_OK; /* free(NULL) não faz nada e não é erro */
        return;
    }
    if (heap_base == NULL) { //NULL não faz nada. Heap não inicializado é erro.
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED: chame mem_init antes de liberar\n");
        ultimo_status = ERR_HEAP_NOT_INITIALIZED;
        return;
    }
    char *p = (char *)ptr; //Se o ponteiro está fora do heap, é erro BAD_POINTER.
    if (p < heap_base + TAM_CAB || p >= heap_base + heap_total) {
        fprintf(stderr, "ERR_BAD_POINTER: ponteiro fora do heap\n");//erro padrão
        ultimo_status = ERR_BAD_POINTER;
        return;
    }
    Bloco *bloco = (Bloco *)(p - TAM_CAB); //Volta 40 bytes para achar o cabeçalho: ptr - TAM_CAB.

    /* TODO: confirmar que b é realmente um bloco da lista */
    Bloco *atual = primeiro; 
    while (atual != NULL && atual != bloco) { //Percorre a lista para confirmar que aquilo é mesmo um cabeçalho. Um ponteiro no meio de um bloco não passa nesse teste.
        atual = atual->proximo;
    }
    if (atual == NULL) {////Se o ponteiro está fora do heap, é erro BAD_POINTER.
        fprintf(stderr, "ERR_BAD_POINTER: ponteiro nao corresponde a um bloco alocado\n"); //stderr = erro padrao
        ultimo_status = ERR_BAD_POINTER;
        return;
    }

    /* TODO: se b->livre, reportar double free e retornar */
    if (bloco->livre) { //Se o bloco já estava livre, é DOUBLE_FREE.
        fprintf(stderr, "ERR_DOUBLE_FREE: bloco ja estava livre\n");//erro padrão
        ultimo_status = ERR_DOUBLE_FREE;
        return;
    }

    /* relatório 3.8: ao liberar, a fragmentação interna do bloco sai do total */
    estatisticas.frag_interna -= bloco->tamanho - bloco->pedido; 
    //Tira do total a fragmentação interna desse bloco e marca como livre.
    /* TODO: marcar como livre */
    bloco->livre = 1;
    bloco->pedido = 0;
    ultimo_status = ALOC_OK;

    /* em meu_free, depois de marcar livre: */
    //Se coalescencia_ativa: funde com o próximo, se estiver livre, e depois com o anterior, se estiver livre.
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
    //chama contar_blocos() e calcula e imprime as métricas. As divisões são protegidas contra divisão por zero.
    if (heap_base == NULL) {
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED\n"); ////NULL não faz nada. Heap não inicializado é erro.
        return;
    }
    contar_blocos();
    size_t n_blocos = estatisticas.n_livres + estatisticas.n_ocupados;

    /* os if evitam divisao por zero */
    // fragmentação externa: 1 - maior_livre/total_livre;
    // taxa de desperdício interno;
    // overhead de cabeçalhos;
    // taxa de falha;
    // blocos examinados por alocação.
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
        fprintf(stderr, "ERR_HEAP_NOT_INITIALIZED\n");////NULL não faz nada. Heap não inicializado é erro.
        return;
    }
    printf("%-10s %-8s %s\n", "Endereco", "Tamanho", "Estado");
    for (Bloco *bloco = primeiro; bloco != NULL; bloco = bloco->proximo) {
        size_t off = (size_t)((char *)bloco - heap_base);

        /* TODO: imprimir off, b->tamanho e LIVRE/OCUPADO */
        printf("%-10zu %-8zu %s\n", off, bloco->tamanho, bloco->livre ? "LIVRE" : "OCUPADO");
        // -10zu exibe o deslocamento do bloco em relação ao início do heap, -8zu exibe o tamanho do bloco, e %s exibe se o bloco está livre ou ocupado
        //imprime uma linha por bloco (offset, tamanho, LIVRE/OCUPADO).

        /* verificação de consistência (roteiro, passo 3) */
        Bloco *prox = bloco->proximo; //confere se proximo->anterior aponta de volta e se o fim de um bloco é exatamente o início do próximo. Se algo não bater, imprime um ALERTA.
        if (prox != NULL) {
            if (prox->anterior != bloco)
                fprintf(stderr, "ALERTA: bloco %zu: proximo->anterior inconsistente\n", off);
            if ((char *)bloco + TAM_CAB + bloco->tamanho != (char *)prox)
                fprintf(stderr, "ALERTA: bloco %zu: fim nao coincide com o proximo bloco\n", off);
        }
    }
}


/* percorre o heap e preenche, em 'estatisticas', a foto atual:
 * quantos blocos livres/ocupados, quanto espaço em cada e o maior livre */
static void contar_blocos(void){
    estatisticas.total_livre = 0;
    estatisticas.total_ocupado = 0;
    estatisticas.maior_livre = 0;
    estatisticas.n_livres = 0;
    estatisticas.n_ocupados = 0;
    for (Bloco *b = primeiro; b != NULL; b = b->proximo) {
        if (b->livre) {
            estatisticas.n_livres++;
            estatisticas.total_livre += b->tamanho;
            if (b->tamanho > estatisticas.maior_livre){
                estatisticas.maior_livre = b->tamanho;
            }
        } else {
            estatisticas.n_ocupados++;
            estatisticas.total_ocupado += b->tamanho;
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

    if (bloco->proximo != NULL){
        bloco->proximo->anterior = novo;
    }
    
    bloco->proximo = novo;

    bloco->tamanho = tamanho;
}

static void coalescencia(Bloco *a, Bloco *b) { //próximo passo
    /* pré-condição: a e b livres e b == a->proximo */
    a->tamanho += TAM_CAB + b->tamanho; 

    /* TODO: religar a->proximo e o ’anterior’ do seguinte de b */
    a->proximo = b->proximo; //junta b dentro de a (b vem logo depois de a, e os dois estão livres).
    
    // a cresce com o cabeçalho e os dados de b;
    // a->proximo passa a pular b;
    // o bloco seguinte passa a apontar de volta para a;
    if (b->proximo != NULL){
        b->proximo->anterior = a;
    }

    reiniciar_next_fit(); /* o next-fit pode estar guardando o bloco b, que deixou de existir */

    /* em meu_free, depois de marcar livre: */
    // if (coalescencia_ativa) {
    /* TODO: caso seguinte livre -> fundir(b, b->proximo) */
    /* TODO: caso anterior livre -> fundir(b->anterior, b) */
    // }
}