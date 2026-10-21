#include <stdio.h>
#include "alocador.h"
#include "estatisticas.h"
#include "estrategias.h"

/*make testes && valgrind --leak-check=full ./testes*/

/* Testes básicos obrigatórios, seguindo os "Verifique antes de seguir" do roteiro.
 * Valores esperados calculados com TAM_CAB = 40 bytes (sizeof(Bloco) com o campo pedido). */

#define CAB(p) ((Bloco *)((char *)(p) - TAM_CAB)) /* cabeçalho a partir do dado */

int total = 0;  /* quantas verificacoes foram feitas */
int falhas = 0; /* quantas deram errado */

void checar(int ok, char *teste);
void teste_inicializacao(void);
void teste_alocacao_simples(void);
void teste_multiplas_alocacoes(void);
void teste_liberacao(void);
void teste_splitting(void);
void teste_coalescencia(void);
void teste_alinhamento(void);
void teste_heap_nao_inicializado(void);
void teste_sem_memoria(void);
void teste_ponteiro_invalido(void);
void teste_double_free(void);
void teste_estrategias(void);
void executar_testes(void);

void checar(int ok, char *teste) {
    total++;
    if (!ok) {
        falhas++;
        printf("FALHOU no teste %s (verificacao %d)\n", teste, total);
    }
}

void teste_inicializacao(void) {
    printf("== teste_inicializacao ==\n");
    mem_init(1000);
    mem_stats();
    /* um único bloco livre com 1000 - TAM_CAB bytes de dados */
    checar(estatisticas.n_livres == 1, "teste_inicializacao");
    checar(estatisticas.n_ocupados == 0, "teste_inicializacao");
    checar(estatisticas.total_livre == 1000 - TAM_CAB, "teste_inicializacao");
    checar(estatisticas.maior_livre == 1000 - TAM_CAB, "teste_inicializacao");
    checar(estatisticas.n_malloc == 0, "teste_inicializacao");
}

void teste_alocacao_simples(void) {
    printf("== teste_alocacao_simples ==\n");
    mem_init(1000);
    void *p = meu_malloc(100);
    checar(p != NULL, "teste_alocacao_simples");
    if (p == NULL) return;
    checar((size_t)p % ALINHAMENTO == 0, "teste_alocacao_simples");
    checar(CAB(p)->tamanho == 104, "teste_alocacao_simples"); /* 100 alinhado para 104 */
    checar(CAB(p)->pedido == 100, "teste_alocacao_simples");
    checar(CAB(p)->livre == 0, "teste_alocacao_simples");
}

void teste_multiplas_alocacoes(void) {
    printf("== teste_multiplas_alocacoes ==\n");
    mem_init(1000);
    char *a = meu_malloc(100);
    char *b = meu_malloc(200);
    char *c = meu_malloc(50);
    checar(a != NULL && b != NULL && c != NULL, "teste_multiplas_alocacoes");
    if (a == NULL || b == NULL || c == NULL) return;
    /* em heap vazio os blocos ficam lado a lado: dados + cabeçalho do seguinte */
    checar(b == a + 104 + TAM_CAB, "teste_multiplas_alocacoes");
    checar(c == b + 200 + TAM_CAB, "teste_multiplas_alocacoes");
    checar(CAB(a)->proximo == CAB(b), "teste_multiplas_alocacoes");
    checar(CAB(b)->anterior == CAB(a), "teste_multiplas_alocacoes");
    checar(CAB(b)->proximo == CAB(c), "teste_multiplas_alocacoes");
    checar(CAB(c)->anterior == CAB(b), "teste_multiplas_alocacoes");
    checar(!CAB(a)->livre && !CAB(b)->livre && !CAB(c)->livre, "teste_multiplas_alocacoes");
}

void teste_liberacao(void) {
    printf("== teste_liberacao ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    checar(a != NULL && b != NULL && c != NULL, "teste_liberacao");
    if (a == NULL || b == NULL || c == NULL) return;
    meu_free(b);
    /* só B muda de estado; vizinhos ocupados, então não há fusão */
    checar(CAB(b)->livre == 1, "teste_liberacao");
    checar(CAB(a)->livre == 0, "teste_liberacao");
    checar(CAB(c)->livre == 0, "teste_liberacao");
    checar(CAB(a)->proximo == CAB(b), "teste_liberacao");
    checar(CAB(b)->proximo == CAB(c), "teste_liberacao");
}

void teste_splitting(void) {
    printf("== teste_splitting ==\n");
    /* caso 1: sobra grande -> divide em ocupado 104 + livre 960 - 104 - 40 = 816 */
    mem_init(1000);
    void *p = meu_malloc(100);
    checar(p != NULL, "teste_splitting");
    if (p == NULL) return;
    Bloco *h = CAB(p);
    Bloco *novo = h->proximo;
    checar(h->tamanho == 104, "teste_splitting");
    checar(novo != NULL, "teste_splitting");
    if (novo == NULL) return;
    checar((char *)novo - (char *)h == 104 + TAM_CAB, "teste_splitting"); /* offset 144 */
    checar(novo->livre == 1, "teste_splitting");
    checar(novo->tamanho == 1000 - TAM_CAB - 104 - TAM_CAB, "teste_splitting");
    checar(novo->anterior == h, "teste_splitting");
    checar(novo->proximo == NULL, "teste_splitting");

    /* caso 2: sobra de 40 < TAM_CAB + MIN_DADOS -> entrega o bloco inteiro */
    mem_init(1000);
    void *q = meu_malloc(920);
    checar(q != NULL, "teste_splitting");
    if (q == NULL) return;
    checar(CAB(q)->tamanho == 1000 - TAM_CAB, "teste_splitting");
    checar(CAB(q)->proximo == NULL, "teste_splitting");
    checar(estatisticas.frag_interna == 40, "teste_splitting"); /* sobra não dividida conta como interna */
}

void teste_coalescencia(void) {
    printf("== teste_coalescencia ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    checar(a != NULL && b != NULL && c != NULL, "teste_coalescencia");
    if (a == NULL || b == NULL || c == NULL) return;

    meu_free(a);
    meu_free(b); /* caso 3: anterior livre -> funde com A */
    checar(CAB(a)->livre == 1, "teste_coalescencia");
    checar(CAB(a)->tamanho == 104 + TAM_CAB + 104, "teste_coalescencia");
    checar(CAB(a)->proximo == CAB(c), "teste_coalescencia");
    checar(CAB(c)->anterior == CAB(a), "teste_coalescencia");

    meu_free(c); /* caso 4: os dois vizinhos livres -> volta ao estado do mem_init */
    checar(CAB(a)->tamanho == 1000 - TAM_CAB, "teste_coalescencia");
    checar(CAB(a)->proximo == NULL, "teste_coalescencia");
    mem_dump();

    /* mesma sequência com a coalescência desligada: os livres ficam separados */
    coalescencia_ativa = 0;
    mem_init(1000);
    a = meu_malloc(100);
    b = meu_malloc(100);
    c = meu_malloc(100);
    checar(a != NULL && b != NULL && c != NULL, "teste_coalescencia");
    if (a != NULL && b != NULL && c != NULL) {
        meu_free(a);
        meu_free(b);
        checar(CAB(a)->tamanho == 104, "teste_coalescencia");
        checar(CAB(a)->proximo == CAB(b), "teste_coalescencia");
        checar(CAB(b)->livre == 1, "teste_coalescencia");
    }
    coalescencia_ativa = 1;
}

void teste_alinhamento(void) {
    printf("== teste_alinhamento ==\n");
    checar(ALINHAR(1) == 8, "teste_alinhamento");
    checar(ALINHAR(8) == 8, "teste_alinhamento");
    checar(ALINHAR(13) == 16, "teste_alinhamento");
    checar(ALINHAR(100) == 104, "teste_alinhamento");
    checar(TAM_CAB % ALINHAMENTO == 0, "teste_alinhamento");

    mem_init(1000);
    void *p1 = meu_malloc(1);
    void *p2 = meu_malloc(13);
    checar(p1 != NULL && p2 != NULL, "teste_alinhamento");
    if (p1 == NULL || p2 == NULL) return;
    checar((size_t)p1 % ALINHAMENTO == 0, "teste_alinhamento");
    checar((size_t)p2 % ALINHAMENTO == 0, "teste_alinhamento");
    checar(CAB(p1)->tamanho == 8, "teste_alinhamento");
    checar(CAB(p2)->tamanho == 16, "teste_alinhamento");
    checar(estatisticas.frag_interna == 7 + 3, "teste_alinhamento");
}

void teste_heap_nao_inicializado(void) {
    printf("== teste_heap_nao_inicializado ==\n");
    mem_destroy();
    size_t antes = estatisticas.n_malloc;
    int local;
    checar(meu_malloc(10) == NULL, "teste_heap_nao_inicializado");
    checar(estatisticas.n_malloc == antes, "teste_heap_nao_inicializado");
    meu_free(&local); /* não pode dar segfault */
    mem_stats();
    mem_dump();
}

void teste_sem_memoria(void) {
    printf("== teste_sem_memoria ==\n");
    mem_init(1000);
    checar(meu_malloc(0) == NULL, "teste_sem_memoria");
    checar(estatisticas.n_malloc == 0, "teste_sem_memoria"); /* malloc(0) não conta como pedido */
    checar(meu_malloc(5000) == NULL, "teste_sem_memoria");   /* maior que o heap inteiro */
    checar(estatisticas.n_falhas == 1, "teste_sem_memoria");

    /* falha por fragmentação externa: 280 e 320 livres, separados por B ocupado */
    mem_init(1000);
    void *a = meu_malloc(280);
    void *b = meu_malloc(280);
    void *c = meu_malloc(280); /* sobra de 40: não divide, C fica com 320 */
    checar(a != NULL && b != NULL && c != NULL, "teste_sem_memoria");
    if (a == NULL || b == NULL || c == NULL) return;
    checar(CAB(c)->tamanho == 320, "teste_sem_memoria");
    meu_free(a);
    meu_free(c);
    checar(meu_malloc(500) == NULL, "teste_sem_memoria"); /* 600 livres no total, maior bloco 320 */
    checar(estatisticas.n_falhas == 1, "teste_sem_memoria");
    checar(estatisticas.n_falhas_frag == 1, "teste_sem_memoria");
}

void teste_ponteiro_invalido(void) {
    printf("== teste_ponteiro_invalido ==\n");
    mem_init(1000);
    char *a = meu_malloc(100);
    void *b = meu_malloc(100);
    checar(a != NULL && b != NULL, "teste_ponteiro_invalido");
    if (a == NULL || b == NULL) return;
    size_t frag_antes = estatisticas.frag_interna;
    int local;

    meu_free(a + 8);  /* endereço no meio de A */
    meu_free(&local); /* endereço fora do heap */
    checar(CAB(a)->livre == 0, "teste_ponteiro_invalido");
    checar(CAB(b)->livre == 0, "teste_ponteiro_invalido");
    checar(estatisticas.frag_interna == frag_antes, "teste_ponteiro_invalido");
}

void teste_double_free(void) {
    printf("== teste_double_free ==\n");
    mem_init(1000);
    void *a = meu_malloc(100);
    void *b = meu_malloc(100);
    void *c = meu_malloc(100);
    checar(a != NULL && b != NULL && c != NULL, "teste_double_free");
    if (a == NULL || b == NULL || c == NULL) return;

    meu_free(b);
    size_t frag_antes = estatisticas.frag_interna;
    meu_free(b); /* deve ser detectado e ignorado */
    checar(CAB(b)->livre == 1, "teste_double_free");
    checar(estatisticas.frag_interna == frag_antes, "teste_double_free");
    checar(estatisticas.frag_interna == 4 + 4, "teste_double_free"); /* só A e C continuam ocupados */
    checar(CAB(a)->proximo == CAB(b), "teste_double_free");
    checar(CAB(b)->proximo == CAB(c), "teste_double_free");
}

void teste_estrategias(void) {
    printf("== teste_estrategias ==\n");
    /* roteiro, passo 4: livres de 104, 56 e um grande no fim, pedido de 40 */
    Estrategia estrategias[] = { FIRST_FIT, BEST_FIT, WORST_FIT };
    for (int i = 0; i < 3; i++) {
        mem_init(1000);
        definir_estrategia(estrategias[i]);
        char *livre1 = meu_malloc(104);
        void *sep1   = meu_malloc(8);   /* separadores ocupados: impedem a fusão */
        char *livre2 = meu_malloc(56);
        char *sep2   = meu_malloc(8);
        checar(livre1 != NULL && sep1 != NULL && livre2 != NULL && sep2 != NULL, "teste_estrategias");
        if (livre1 == NULL || sep1 == NULL || livre2 == NULL || sep2 == NULL) break;
        char *livre3 = sep2 + 8 + TAM_CAB; /* dados do bloco livre final (624 bytes) */
        meu_free(livre1);
        meu_free(livre2);

        char *p = meu_malloc(40);
        if (estrategias[i] == FIRST_FIT) checar(p == livre1, "teste_estrategias"); /* primeiro que serve */
        if (estrategias[i] == BEST_FIT)  checar(p == livre2, "teste_estrategias"); /* menor que serve */
        if (estrategias[i] == WORST_FIT) checar(p == livre3, "teste_estrategias"); /* maior livre */
    }
    definir_estrategia(FIRST_FIT);
}

void executar_testes(void) {
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
}

int main(void) {
    executar_testes();
    mem_destroy();
    printf("\n%d/%d verificacoes OK\n", total - falhas, total);
    return falhas == 0 ? 0 : 1;
}
