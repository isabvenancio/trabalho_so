#compilador e flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11 -MMD -MP -Isrc
#objetovs vao para build/src e build/benchmarks
BUILD   = build

# biblioteca do alocador (tudo menos os programas com main)
# arquivos fontes e objetos
SRCS = src/alocador.c src/estrategias.c src/estatisticas.c
OBJS = $(SRCS:%.c=$(BUILD)/%.o)

# programas: cada um tem o seu main
CSV       ?= resultados/comparativo.csv
EXEC      = alocador
TESTES    = testes
DRIVER    = driver
WORKLOADS = workload1 workload2 workload3

# arquivos executáveis "target" que serão gerados
BINS      = $(EXEC) $(TESTES) $(DRIVER) $(WORKLOADS)

.PHONY: all test exemplos bench analise sanitize clean
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

$(BUILD)/%.o: %.c
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

# make sanitize -> recompila tudo com AddressSanitizer/UBSan e roda os testes
sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" test

clean:
	rm -rf $(BUILD) $(BINS)

-include $(OBJS:.o=.d) $(BUILD)/src/main.d $(BUILD)/src/testes.d $(BUILD)/src/driver.d $(WORKLOADS:%=$(BUILD)/benchmarks/%.d)
