# make gera o alocador (a partir do main.c)
CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -Isrc

# código do alocador, usado por todos os programas
SRCS    = src/alocador.c src/estrategias.c src/estatisticas.c
HEADERS = src/alocador.h src/estrategias.h src/estatisticas.h

CSV       ?= resultados/comparativo.csv
WORKLOADS = workload1 workload2 workload3

.PHONY: all test exemplos bench analise valgrind clean
all: alocador

# cada programa é compilado de uma vez: o seu main + o código do alocador
alocador: src/main.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ src/main.c $(SRCS)

testes: src/testes.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ src/testes.c $(SRCS)

driver: src/driver.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ src/driver.c $(SRCS)

# workloadN -> benchmarks/workloadN.c
$(WORKLOADS): %: benchmarks/%.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

# referência: mesmo padrão de pedidos com o malloc real da biblioteca do C. Usa -O2 porque a
# biblioteca do C já vem otimizada, e comparar com o nosso sem otimização seria injusto
referencia_malloc: CFLAGS += -O2
referencia_malloc: benchmarks/referencia_malloc.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

# make test -> roda os testes obrigatórios
test: testes
	./testes

# make exemplos -> roda cada exemplos/entradaN.txt e compara com exemplos/saidaN.txt
exemplos: driver
	@ok=0; falha=0; \
	for e in exemplos/entrada*.txt; do \
	  s=$$(echo $$e | sed 's/entrada/saida/'); \
	  if ./driver $$e 2>&1 | diff -u $$s - > /dev/null; then echo "OK    $$e"; ok=$$((ok+1)); \
	  else echo "FALHA $$e"; ./driver $$e 2>&1 | diff -u $$s - | head -20; falha=$$((falha+1)); fi; \
	done; echo "$$ok ok, $$falha falha(s)"; [ $$falha -eq 0 ]

# make bench -> escreve o cabeçalho do CSV e acrescenta (>>) as linhas que cada workload imprime
# 2> /dev/null esconde as mensagens de erro do alocador: nos workloads, as falhas são esperadas
bench: $(WORKLOADS) referencia_malloc
	@mkdir -p resultados
	@echo "workload,estrategia,coalescencia,operacao,heap_size,alocacoes_ok,falhas,falhas_frag,maior_livre,total_livre,frag_interna,overhead,blocos_examinados,tempo_alloc_ns,tempo_free_ns" > $(CSV)
	@echo "workload,estrategia,coalescencia,endereco,bytes,livre" > resultados/blocos.csv
	@for w in $(WORKLOADS); do ./$$w >> $(CSV) 2> /dev/null || exit 1; done
	@./referencia_malloc > resultados/tempo_malloc.csv 2> /dev/null
	@echo "dados gravados em $(CSV)"

# make analise -> gera tabelas e gráficos a partir do CSV
analise:
	python3 src/analise.py $(CSV)

# make valgrind -> roda os testes, os exemplos e os workloads dentro do valgrind
# a saída é escondida; se der erro, rode o comando mostrado na mensagem para ver os detalhes
VALGRIND = valgrind -q --leak-check=full --error-exitcode=1
valgrind: testes driver $(WORKLOADS) referencia_malloc
	@$(VALGRIND) ./testes > /dev/null 2>&1 || { echo "valgrind achou erro: rode $(VALGRIND) ./testes"; exit 1; }
	@for e in exemplos/entrada*.txt; do $(VALGRIND) ./driver $$e > /dev/null 2>&1 || { echo "valgrind achou erro: rode $(VALGRIND) ./driver $$e"; exit 1; }; done
	@for w in $(WORKLOADS) referencia_malloc; do $(VALGRIND) ./$$w > /dev/null 2>&1 || { echo "valgrind achou erro: rode $(VALGRIND) ./$$w"; exit 1; }; done
	@echo "valgrind: nenhum erro de memoria nem vazamento"

clean:
	rm -f alocador testes driver $(WORKLOADS) referencia_malloc
