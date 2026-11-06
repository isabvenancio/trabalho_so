#ifndef ALOCADOR_H 
#define ALOCADOR_H

#include <stddef.h> /* size_t */

#define ALINHAMENTO 8 //todo tamanho entregue é múltiplo de 8 bytes
#define ALINHAR(n) ((((n) + ALINHAMENTO - 1) / ALINHAMENTO) * ALINHAMENTO) /* arredonda para o próximo múltiplo de 8: 5 vira 8, 100 vira 104 */
#define TAM_CAB ALINHAR(sizeof(Bloco)) //tamanho do cabeçalho de cada bloco: 40 bytes
#define MIN_DADOS 8 /* menor área de dados que vale a pena criar no splitting (valor padrão) */

// cabeçalho de cada bloco, guardado dentro do próprio heap, logo antes dos dados
// heap: [cab|dados....][cab|dados..][cab|dados...........]

typedef struct bloco {
    size_t tamanho;//bytes de dados do bloco, sem contar o cabeçalho
    size_t pedido; /* quanto o usuário pediu (stats): tamanho - pedido = fragmentação interna */
    int livre; //1 = livre, 0 = ocupado
    struct bloco *proximo;//bloco seguinte na memória (NULL no último)
    struct bloco *anterior;//bloco anterior na memória (NULL no primeiro)
} Bloco;

typedef enum status_aloc {
    ALOC_OK = 0,
    ERR_HEAP_NOT_INITIALIZED,
    ERR_OUT_OF_MEMORY,     /* não há espaço livre suficiente nem somando tudo */
    ERR_FRAGMENTATION,     /* há espaço livre total, mas nenhum bloco contíguo */
    ERR_BAD_POINTER,
    ERR_DOUBLE_FREE
} AlocStatus;

extern size_t min_dados;/* menor sobra que vira um bloco novo no splitting (padrão: MIN_DADOS) */
extern int coalescencia_ativa; /* 1 = funde vizinhos livres no free; 0 = não funde */
extern char *heap_base;/* início do heap simulado (NULL antes do mem_init) */
extern AlocStatus ultimo_status;/* resultado da última chamada de meu_malloc ou meu_free */

void mem_init(size_t heap_size);
void mem_destroy(void);
void* meu_malloc(size_t tamanho);
void meu_free(void* ptr);
void mem_stats(void);
void mem_dump(void);
void mem_dump_csv(const char *nome);/* acrescenta os blocos atuais em resultados/blocos.csv */
void contar_blocos(void);/* atualiza a foto atual do heap em 'estatisticas' */

#endif
