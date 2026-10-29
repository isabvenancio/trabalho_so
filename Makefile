#make gera o alocador (a partir do main.c).
#compilador e flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11 -Isrc
#objetovs vao para build/src e build/benchmarks
BUILD   = build

# biblioteca do alocador (tudo menos os programas com main)
# arquivos fontes e objetos
SRCS = src/alocador.c src/estrategias.c src/estatisticas.c
OBJS = $(SRCS:%.c=$(BUILD)/%.o)
HEADERS = src/alocador.h src/estrategias.h src/estatisticas.h

# programas: cada um tem o seu main
CSV       ?= resultados/comparativo.csv
EXEC      = alocador
TESTES    = testes
DRIVER    = driver
WORKLOADS = workload1 workload2 workload3

# arquivos executáveis "target" que serão gerados
BINS      = $(EXEC) $(TESTES) $(DRIVER) $(WORKLOADS)

.PHONY: all test exemplos bench analise valgrind clean
all: $(EXEC)

$(EXEC): $(BUILD)/src/main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTES): $(BUILD)/src/testes.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(DRIVER): $(BUILD)/src/driver.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# workloadN -> benchmarks/workloadN.c
$(WORKLOADS): %: $(BUILD)/benchmarks/%.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# compila cada .c em build/...; se qualquer header mudar, recompila
$(BUILD)/%.o: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# make test -> roda os testes obrigatórios
test: $(TESTES)
	./$(TESTES)

# make exemplos -> roda cada exemplos/entradaN.txt e compara com exemplos/saidaN.txt
exemplos: $(DRIVER)
	@ok=0; falha=0; \
	for e in exemplos/entrada*.txt; do \
	  s=$$(echo $$e | sed 's/entrada/saida/'); \
	  if ./$(DRIVER) $$e 2>&1 | diff -u $$s - > /dev/null; then echo "OK    $$e"; ok=$$((ok+1)); \
	  else echo "FALHA $$e"; ./$(DRIVER) $$e 2>&1 | diff -u $$s - | head -20; falha=$$((falha+1)); fi; \
	done; echo "$$ok ok, $$falha falha(s)"; [ $$falha -eq 0 ]

# make bench -> roda os 3 workloads (cada um acrescenta linhas em resultados/comparativo.csv)
bench: $(WORKLOADS)
	@mkdir -p resultados
	@for w in $(WORKLOADS); do ./$$w || exit 1; done

# make analise -> gera tabelas e gráficos a partir do CSV
analise:
	python3 src/analise.py $(CSV)

# make valgrind -> roda os testes e todos os exemplos dentro do valgrind
# (--error-exitcode=1: o programa "falha" se o valgrind achar erro; a saída é escondida,
#  então, se der erro, rode o comando mostrado na mensagem para ver os detalhes)
VALGRIND = valgrind -q --leak-check=full --error-exitcode=1
valgrind: $(TESTES) $(DRIVER)
	@$(VALGRIND) ./$(TESTES) > /dev/null 2>&1 || { echo "valgrind achou erro nos testes: rode $(VALGRIND) ./$(TESTES)"; exit 1; }
	@for e in exemplos/entrada*.txt; do $(VALGRIND) ./$(DRIVER) $$e > /dev/null 2>&1 || { echo "valgrind achou erro em $$e: rode $(VALGRIND) ./$(DRIVER) $$e"; exit 1; }; done
	@echo "valgrind: nenhum erro de memoria nem vazamento"

clean:
	rm -rf $(BUILD) $(BINS)
