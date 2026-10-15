#compilador e flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11 -MMD -MP -Isrc
BUILD   = build #objetovs vao para build/src e build/benchmarks

# biblioteca do alocador (tudo menos os programas com main)
# arquivos fontes e objetos
SRCS = src/alocador.c src/estrategias.c src/estatisticas.c
OBJS = $(SRCS:%.c=$(BUILD)/%.o)

# programas: cada um tem o seu main
CSV       ?= resultados/comparativo.csv
EXEC      = alocador
TESTES    = testes
WORKLOADS = workload1 workload2 workload3
BINS      = $(EXEC) $(TESTES) $(WORKLOADS) # arquivos executáveis "target" que serão gerados

.PHONY: all test bench analise sanitize clean
all: $(EXEC)

$(EXEC): $(BUILD)/src/main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTES): $(BUILD)/src/testes.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# workloadN -> benchmarks/workloadN.c
$(WORKLOADS): %: $(BUILD)/benchmarks/%.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# make test -> roda os testes obrigatórios
test: $(TESTES)
	./$(TESTES)

# make bench -> roda os 3 workloads (cada um acrescenta linhas em resultados/comparativo.csv)
bench: $(WORKLOADS)
	@mkdir -p resultados
	@for w in $(WORKLOADS); do ./$$w || exit 1; done

# make analise -> gera tabelas e gráficos a partir do CSV
analise:
	python3 src/analise.py $(CSV)

# make sanitize -> recompila tudo com AddressSanitizer/UBSan e roda os testes
sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" test

clean:
	rm -rf $(BUILD) $(BINS)

-include $(OBJS:.o=.d) $(BUILD)/src/main.d $(BUILD)/src/testes.d $(WORKLOADS:%=$(BUILD)/benchmarks/%.d)
