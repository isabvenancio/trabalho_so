/* Testes básicos obrigatórios, seguindo os "Verifique antes de seguir" do roteiro do TSO.
 * Valores esperados calculados com TAM_CAB = 40 bytes (sizeof(Bloco) com o campo pedido).
 * Para rodar: make test (ou make valgrind, que roda dentro do valgrind) */
#include <stdio.h>
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

#define CAB(p) ((Bloco *)((char *)(p) - TAM_CAB)) /* cabeçalho a partir do dado */
//CAB(p) = macro que volta do ponteiro de dados para o cabeçalho, para os testes conferirem os campos.

int total = 0;  /* quantas verificacoes foram feitas */
int falhas = 0; /* quantas deram errado */

void checar(int ok, const char *teste) { //conta uma verificação e imprime se falhar.
    total++;
    if (!ok) {
        falhas++;
        printf("FALHOU no teste %s (verificacao %d)\n", teste, total);
    }
}

//CHECAR(condição) chama checar com o nome da função atual: __func__ é o nome da função, que o C preenche sozinho
#define CHECAR(ok) checar((ok), __func__)

void teste_inicializacao(void) {
    printf("== teste_inicializacao ==\n");
    mem_init(1000);
    mem_stats();
    /* um único bloco livre com 1000 - TAM_CAB bytes de dados */
    CHECAR(estatisticas.n_livres == 1);
    CHECAR(estatisticas.n_ocupados == 0);
    CHECAR(estatisticas.total_livre == 1000 - TAM_CAB);
    CHECAR(estatisticas.maior_livre == 1000 - TAM_CAB);
    CHECAR(estatisticas.n_malloc == 0);
}

void teste_alocacao_simples(void) {
    printf("== teste_alocacao_simples ==\n");
    mem_init(1000);
    void *p = meu_malloc(100);
    CHECAR(p != NULL);
    if (p == NULL) return;
    CHECAR((size_t)p % ALINHAMENTO == 0);
    CHECAR(CAB(p)->tamanho == 104); /* 100 alinhado para 104 */
    CHECAR(CAB(p)->pedido == 100);
    CHECAR(CAB(p)->livre == 0);
}

void teste_multiplas_alocacoes(void) {
    printf("== teste_multiplas_alocacoes ==\n");
    mem_init(1000);
    char *a = meu_malloc(100);
    char *b = meu_malloc(200);
    char *c = meu_malloc(50);
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a == NULL || b == NULL || c == NULL) return;
    /* em heap vazio os blocos ficam lado a lado: dados + cabeçalho do seguinte */
    CHECAR(b == a + 104 + TAM_CAB);
    CHECAR(c == b + 200 + TAM_CAB);
    CHECAR(CAB(a)->proximo == CAB(b));
    CHECAR(CAB(b)->anterior == CAB(a));
    CHECAR(CAB(b)->proximo == CAB(c));
    CHECAR(CAB(c)->anterior == CAB(b));
    CHECAR(!CAB(a)->livre && !CAB(b)->livre && !CAB(c)->livre);
}

void teste_liberacao(void) {
    printf("== teste_liberacao ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a == NULL || b == NULL || c == NULL) return;
    meu_free(b);
    /* só B muda de estado; vizinhos ocupados, então não há fusão */
    CHECAR(CAB(b)->livre == 1);
    CHECAR(CAB(a)->livre == 0);
    CHECAR(CAB(c)->livre == 0);
    CHECAR(CAB(a)->proximo == CAB(b));
    CHECAR(CAB(b)->proximo == CAB(c));
}

void teste_splitting(void) {
    printf("== teste_splitting ==\n");
    /* caso 1: sobra grande -> divide em ocupado 104 + livre 960 - 104 - 40 = 816 */
    mem_init(1000);
    void *p = meu_malloc(100);
    CHECAR(p != NULL);
    if (p == NULL) return;
    Bloco *h = CAB(p);
    Bloco *novo = h->proximo;
    CHECAR(h->tamanho == 104);
    CHECAR(novo != NULL);
    if (novo == NULL) return;
    CHECAR((char *)novo - (char *)h == 104 + TAM_CAB); /* offset 144 */
    CHECAR(novo->livre == 1);
    CHECAR(novo->tamanho == 1000 - TAM_CAB - 104 - TAM_CAB);
    CHECAR(novo->anterior == h);
    CHECAR(novo->proximo == NULL);

    /* caso 2: sobra de 40 < TAM_CAB + MIN_DADOS -> entrega o bloco inteiro */
    mem_init(1000);
    void *q = meu_malloc(920);
    CHECAR(q != NULL);
    if (q == NULL) return;
    CHECAR(CAB(q)->tamanho == 1000 - TAM_CAB);
    CHECAR(CAB(q)->proximo == NULL);
    CHECAR(estatisticas.frag_interna == 40); /* sobra não dividida conta como interna */

    /* caso 3: mesma sobra, valores diferentes de min_dados. Bloco de 960, pedido de 800: sobra de 160 */
    mem_init(1000);
    min_dados = 128; /* 160 >= 40 + 128 é falso: não divide */
    void *r = meu_malloc(800);
    CHECAR(r != NULL && CAB(r)->proximo == NULL);
    mem_init(1000);
    min_dados = 120; /* 160 >= 40 + 120 é verdadeiro: divide */
    r = meu_malloc(800);
    CHECAR(r != NULL && CAB(r)->proximo != NULL);
    min_dados = MIN_DADOS;
}

void teste_coalescencia(void) {
    printf("== teste_coalescencia ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a == NULL || b == NULL || c == NULL) return;

    meu_free(a);
    meu_free(b); /* caso 3: anterior livre -> funde com A */
    CHECAR(CAB(a)->livre == 1);
    CHECAR(CAB(a)->tamanho == 104 + TAM_CAB + 104);
    CHECAR(CAB(a)->proximo == CAB(c));
    CHECAR(CAB(c)->anterior == CAB(a));

    meu_free(c); /* caso 4: os dois vizinhos livres -> volta ao estado do mem_init */
    CHECAR(CAB(a)->tamanho == 1000 - TAM_CAB);
    CHECAR(CAB(a)->proximo == NULL);
    mem_dump();

    /* mesma sequência com a coalescência desligada: os livres ficam separados */
    coalescencia_ativa = 0;
    mem_init(1000);
    a = meu_malloc(100);
    b = meu_malloc(100);
    c = meu_malloc(100);
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a != NULL && b != NULL && c != NULL) {
        meu_free(a);
        meu_free(b);
        CHECAR(CAB(a)->tamanho == 104);
        CHECAR(CAB(a)->proximo == CAB(b));
        CHECAR(CAB(b)->livre == 1);
    }
    coalescencia_ativa = 1;
}

void teste_alinhamento(void) {
    printf("== teste_alinhamento ==\n");
    CHECAR(ALINHAR(1) == 8);
    CHECAR(ALINHAR(8) == 8);
    CHECAR(ALINHAR(13) == 16);
    CHECAR(ALINHAR(100) == 104);
    CHECAR(TAM_CAB % ALINHAMENTO == 0);

    mem_init(1000);
    void *p1 = meu_malloc(1);
    void *p2 = meu_malloc(13);
    CHECAR(p1 != NULL && p2 != NULL);
    if (p1 == NULL || p2 == NULL) return;
    CHECAR((size_t)p1 % ALINHAMENTO == 0);
    CHECAR((size_t)p2 % ALINHAMENTO == 0);
    CHECAR(CAB(p1)->tamanho == 8);
    CHECAR(CAB(p2)->tamanho == 16);
    CHECAR(estatisticas.frag_interna == 7 + 3);
}

void teste_heap_nao_inicializado(void) {
    printf("== teste_heap_nao_inicializado ==\n");
    mem_destroy();
    size_t antes = estatisticas.n_malloc;
    int local;
    CHECAR(meu_malloc(10) == NULL);
    CHECAR(ultimo_status == ERR_HEAP_NOT_INITIALIZED);
    CHECAR(estatisticas.n_malloc == antes);
    meu_free(&local); /* não pode dar segfault */
    CHECAR(ultimo_status == ERR_HEAP_NOT_INITIALIZED);
    mem_stats();
    mem_dump();
}

void teste_sem_memoria(void) {
    printf("== teste_sem_memoria ==\n");
    mem_init(1000);
    CHECAR(meu_malloc(0) == NULL);
    CHECAR(estatisticas.n_malloc == 0); /* malloc(0) não conta como pedido */
    CHECAR(meu_malloc(5000) == NULL);   /* maior que o heap inteiro */
    CHECAR(ultimo_status == ERR_OUT_OF_MEMORY);
    CHECAR(estatisticas.n_falhas == 1);

    /* falha por fragmentação externa: 280 e 320 livres, separados por B ocupado */
    mem_init(1000);
    void *a = meu_malloc(280);
    void *b = meu_malloc(280);
    void *c = meu_malloc(280); /* sobra de 40: não divide, C fica com 320 */
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a == NULL || b == NULL || c == NULL) return;
    CHECAR(CAB(c)->tamanho == 320);
    meu_free(a);
    meu_free(c);
    CHECAR(meu_malloc(500) == NULL); /* 600 livres no total, maior bloco 320 */
    CHECAR(ultimo_status == ERR_FRAGMENTATION);
    CHECAR(estatisticas.n_falhas == 1);
    CHECAR(estatisticas.n_falhas_frag == 1);
}

void teste_ponteiro_invalido(void) {
    printf("== teste_ponteiro_invalido ==\n");
    mem_init(1000);
    char *a = meu_malloc(100);
    void *b = meu_malloc(100);
    CHECAR(a != NULL && b != NULL);
    if (a == NULL || b == NULL) return;
    size_t frag_antes = estatisticas.frag_interna;
    int local;

    meu_free(a + 8);  /* endereço no meio de A */
    CHECAR(ultimo_status == ERR_BAD_POINTER);
    meu_free(&local); /* endereço fora do heap */
    CHECAR(ultimo_status == ERR_BAD_POINTER);
    CHECAR(CAB(a)->livre == 0);
    CHECAR(CAB(b)->livre == 0);
    CHECAR(estatisticas.frag_interna == frag_antes);
}

void teste_double_free(void) {
    printf("== teste_double_free ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    CHECAR(a != NULL && b != NULL && c != NULL);
    if (a == NULL || b == NULL || c == NULL) return;

    meu_free(b);
    CHECAR(ultimo_status == ALOC_OK);
    size_t frag_antes = estatisticas.frag_interna;
    meu_free(b); /* deve ser detectado e ignorado */
    CHECAR(ultimo_status == ERR_DOUBLE_FREE);
    CHECAR(CAB(b)->livre == 1);
    CHECAR(estatisticas.frag_interna == frag_antes);
    CHECAR(estatisticas.frag_interna == 4 + 4); /* só A e C continuam ocupados */
    CHECAR(CAB(a)->proximo == CAB(b));
    CHECAR(CAB(b)->proximo == CAB(c));
}

void teste_estrategias(void) {
    printf("== teste_estrategias ==\n");
    /* roteiro, passo 4: livres de 104, 56 e um grande no fim, pedido de 40 */
    Estrategia estrategias[] = { FIRST_FIT, BEST_FIT, WORST_FIT };
    for (int i = 0; i < 3; i++) {
        mem_init(1000);
        estrategia_atual = estrategias[i];
        char *livre1 = meu_malloc(104);
        void *sep1   = meu_malloc(8);   /* separadores ocupados: impedem a fusão */
        char *livre2 = meu_malloc(56);
        char *sep2   = meu_malloc(8);
        CHECAR(livre1 != NULL && sep1 != NULL && livre2 != NULL && sep2 != NULL);
        if (livre1 == NULL || sep1 == NULL || livre2 == NULL || sep2 == NULL) break;
        char *livre3 = sep2 + 8 + TAM_CAB; /* dados do bloco livre final (624 bytes) */
        meu_free(livre1);
        meu_free(livre2);

        char *p = meu_malloc(40);
        if (estrategias[i] == FIRST_FIT) CHECAR(p == livre1); /* primeiro que serve */
        if (estrategias[i] == BEST_FIT)  CHECAR(p == livre2); /* menor que serve */
        if (estrategias[i] == WORST_FIT) CHECAR(p == livre3); /* maior livre */
    }
    estrategia_atual = FIRST_FIT;
}

void teste_next_fit(void) {
    printf("== teste_next_fit ==\n");
    estrategia_atual = NEXT_FIT;

    /* continua de onde parou: ignora o buraco do início e usa o bloco livre do fim */
    mem_init(1000);
    char *a = meu_malloc(104);
    void *sep = meu_malloc(8); /* separador ocupado: impede a fusão */
    char *fim = (char *)sep + 8 + TAM_CAB; /* dados do bloco livre do fim (760 bytes) */
    CHECAR(a != NULL && sep != NULL);
    if (a == NULL || sep == NULL) return;
    meu_free(a);
    char *p = meu_malloc(40);
    CHECAR(p == fim);

    /* dá a volta: o fim do heap fica cheio e o próximo pedido usa o buraco do início */
    void *resto = meu_malloc(CAB(p)->proximo->tamanho);
    CHECAR(resto != NULL);
    CHECAR(meu_malloc(40) == a);

    /* nenhum bloco serve: precisa terminar a volta e devolver NULL, sem laço infinito */
    CHECAR(meu_malloc(900) == NULL);

    /* o bloco guardado é fundido: a busca continua do bloco que o absorveu */
    mem_init(1000);
    void *x = meu_malloc(100);
    void *y = meu_malloc(100); /* o next-fit guarda y */
    CHECAR(x != NULL && y != NULL);
    if (x == NULL || y == NULL) return;
    meu_free(x);
    meu_free(y); /* y é fundido em x */
    CHECAR(meu_malloc(8) == x);

    estrategia_atual = FIRST_FIT;
}

void executar_testes(void) { //roda todos os testes, na ordem do roteiro
    teste_inicializacao();
    teste_alocacao_simples();
    teste_multiplas_alocacoes();
    teste_liberacao();
    teste_splitting();
    teste_coalescencia();
    teste_alinhamento();
    teste_heap_nao_inicializado();
    teste_sem_memoria();
    teste_ponteiro_invalido();
    teste_double_free();
    teste_estrategias();
    teste_next_fit();
}

int main(void) {
    executar_testes();
    mem_destroy(); //libera o heap do último teste

    printf("\n%d/%d verificacoes OK\n", total - falhas, total);
    return falhas == 0 ? 0 : 1; //devolve 1 se algo falhou: o make test acusa erro
}
