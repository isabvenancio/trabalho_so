# make gera o alocador (a partir do main.c)
# make test, make exemplos, make valgrind, make bench, make analise, make clean
# Outros testes:
# gcc -Wall -Wextra -Wpedantic -Wshadow -std=c11 -Isrc -fsyntax-only src/*.c
# for f in src/alocador.c src/estrategias.c src/driver.c src/testes.c; do gcc -std=c11 -Isrc -fanalyzer -c $f -o /dev/null; done
# ./driver /tmp/aleatorio_best.txt 2>&1 | tail -8
CC     = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -Isrc

# código do alocador, usado por todos os programas
SRCS    = src/alocador.c src/estrategias.c src/estatisticas.c
HEADERS = src/alocador.h src/estrategias.h src/estatisticas.h

CSV        ?= resultados/comparativo.csv
BENCHMARKS = workload1 workload2 workload3 referencia_malloc min_dados # um programa para cada arquivo de benchmarks/

.PHONY: all test exemplos bench analise valgrind clean
all: alocador

# cada programa é compilado de uma vez: o seu main + o código do alocador
# ($@ é o nome do programa e $< é o primeiro arquivo da lista, o que tem o main)
alocador: src/main.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

testes: src/testes.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

driver: src/driver.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

# benchmarks: o programa X vem de benchmarks/X.c
$(BENCHMARKS): %: benchmarks/%.c $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SRCS)

# o referencia_malloc compara com o malloc real, que já vem otimizado da biblioteca do C:
# por isso só ele usa -O2, senão a comparação seria injusta
referencia_malloc: CFLAGS += -O2

# make test -> roda os testes obrigatórios
test: testes
	./testes

# make exemplos -> roda cada exemplos/entradaN.txt e compara com exemplos/saidaN.txt
# (s é o nome da saída esperada; diff -u mostra as diferenças; no fim, conta quantos deram certo)
exemplos: driver
	@ok=0; falha=0; \
	for e in exemplos/entrada*.txt; do \
	  s=$$(echo $$e | sed 's/entrada/saida/'); \
	  if ./driver $$e 2>&1 | diff -u $$s - > /dev/null; then echo "OK    $$e"; ok=$$((ok+1)); \
	  else echo "FALHA $$e"; ./driver $$e 2>&1 | diff -u $$s - | head -20; falha=$$((falha+1)); fi; \
	done; echo "$$ok ok, $$falha falha(s)"; [ $$falha -eq 0 ]

# make bench -> escreve o cabeçalho dos CSVs e acrescenta (>>) as linhas que cada workload imprime
# 2> /dev/null esconde as mensagens de erro do alocador: nos workloads, as falhas são esperadas
bench: $(BENCHMARKS)
	@mkdir -p resultados
	@echo "workload,estrategia,coalescencia,operacao,heap_size,alocacoes_ok,falhas,falhas_frag,maior_livre,total_livre,frag_interna,overhead,blocos_examinados,tempo_alloc_ns,tempo_free_ns" > $(CSV)
	@echo "workload,estrategia,coalescencia,endereco,bytes,livre" > resultados/blocos.csv
	@for w in workload1 workload2 workload3; do ./$$w >> $(CSV) 2> /dev/null || exit 1; done
	@./referencia_malloc > resultados/tempo_malloc.csv 2> /dev/null
	@./min_dados > resultados/min_dados.csv 2> /dev/null
	@echo "dados gravados em resultados/"

# make analise -> gera tabelas e gráficos a partir dos CSVs
analise:
	python3 src/analise.py $(CSV)

# make valgrind -> roda os testes e os exemplos dentro do valgrind
# (os benchmarks ficam de fora: eles gravam em resultados/ e o valgrind duplicaria as linhas do blocos.csv)
# a saída é escondida; se der erro, rode o comando mostrado na mensagem para ver os detalhes
VALGRIND = valgrind -q --leak-check=full --error-exitcode=1
valgrind: testes driver
	@$(VALGRIND) ./testes > /dev/null 2>&1 || { echo "valgrind achou erro: rode $(VALGRIND) ./testes"; exit 1; }
	@for e in exemplos/entrada*.txt; do $(VALGRIND) ./driver $$e > /dev/null 2>&1 || { echo "valgrind achou erro: rode $(VALGRIND) ./driver $$e"; exit 1; }; done
	@echo "valgrind: nenhum erro de memoria nem vazamento"

# make clean -> apaga só os programas compilados (os resultados ficam)
clean:
	rm -f alocador testes driver $(BENCHMARKS)
