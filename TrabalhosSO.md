# Trabalhos Práticos — Sistemas Operacionais

Disciplina: Sistemas Operacionais — Ciência da Computação
Linguagem sugerida: **C** (POSIX/Linux), salvo indicação contrária

---

## Sumário

1. [Shell Simplificado (Mini-Shell)](#1-shell-simplificado-mini-shell)
2. [Simulador de Escalonador de CPU](#2-simulador-de-escalonador-de-cpu)
3. [Simulador de Paginação](#3-simulador-de-paginação)
4. [Alocador de Memória Customizado](#4-alocador-de-memória-customizado)
5. [Problemas Clássicos de Sincronização](#5-problemas-clássicos-de-sincronização)

---

## 1. Shell Simplificado (Mini-Shell)

### Objetivo
Compreender criação e gerência de processos, descritores de arquivo e comunicação entre processos através da implementação de um interpretador de comandos.

### Conceitos exercitados
- `fork()`, `exec*()`, `wait()`/`waitpid()`
- Descritores de arquivo e redirecionamento (`dup2`)
- Pipes (`pipe()`)
- Sinais (`SIGINT`, `SIGCHLD`) e processos em background

### Especificação funcional

O programa deve implementar um laço REPL (*read-eval-print loop*) que:

1. Exibe um prompt (ex.: `mysh> `)
2. Lê uma linha de comando
3. Faz o *parsing* da linha (tokenização respeitando aspas)
4. Executa o comando conforme as regras abaixo
5. Retorna ao passo 1 até receber `exit`

### Requisitos obrigatórios

| Funcionalidade | Exemplo | Descrição |
|---|---|---|
| Comandos simples | `ls -la` | Executar via `fork` + `execvp` |
| Comandos internos (*builtins*) | `cd`, `exit`, `pwd`, `export` | Não devem criar processo filho |
| Redirecionamento de saída | `ls > out.txt` | Truncar arquivo |
| Redirecionamento de append | `ls >> out.txt` | Anexar ao final |
| Redirecionamento de entrada | `sort < dados.txt` | Ler de arquivo |
| Pipes simples | `ls -l \| grep .c` | Um pipe |
| Pipes múltiplos (desejável) | `cat f \| sort \| uniq \| wc -l` | N pipes encadeados |
| Execução em background | `sleep 10 &` | Não bloquear o shell; reportar PID |
| Tratamento de `Ctrl+C` | — | Não deve matar o shell, apenas o processo filho ativo |
| Histórico simples (desejável) | `!!` | Repetir último comando |

### Requisitos não funcionais
- Tratar erros de `fork`/`exec` sem derrubar o shell.
- Evitar processos "zumbis" (usar `waitpid` com `WNOHANG` para background, tratamento de `SIGCHLD`).
- Limite razoável de argumentos (ex.: 64) e tamanho de linha (ex.: 1024 bytes), configurável via `#define`.

### Estrutura de entrega
```
mini-shell/
├── src/
│   ├── main.c
│   ├── parser.c / parser.h
│   ├── executor.c / executor.h
│   └── builtins.c / builtins.h
├── Makefile
├── testes/
│   └── casos_de_teste.txt
└── RELATORIO.pdf
```

### Critérios de avaliação
| Critério | Peso |
|---|---|
| Execução de comandos simples | 20% |
| Redirecionamentos | 20% |
| Pipes | 25% |
| Background e sinais | 15% |
| Builtins e robustez (tratamento de erros) | 10% |
| Relatório (decisões de projeto, dificuldades) | 10% |

### Perguntas para o relatório
- Como seu shell trata comandos inexistentes?
- O que acontece com os descritores de arquivo herdados após múltiplos `dup2` em uma cadeia de pipes?
- Por que é necessário fechar as pontas não usadas dos pipes no processo pai e nos filhos?

---

## 2. Simulador de Escalonador de CPU

### Objetivo
Implementar e comparar experimentalmente diferentes algoritmos de escalonamento de processos, avaliando seu impacto em métricas de desempenho.

### Conceitos exercitados
- Estados de processo e contexto de execução
- Métricas de escalonamento (espera, turnaround, resposta)
- Trade-offs entre throughput, justiça (*fairness*) e tempo de resposta

### Entrada do simulador

Arquivo de texto no formato:
```
# PID  Chegada  Burst  Prioridade
1      0        8      2
2      1        4      1
3      2        9      3
4      3        5      2
```

### Algoritmos a implementar

| Algoritmo | Sigla | Preemptivo? | Observação |
|---|---|---|---|
| First-Come, First-Served | FCFS | Não | Baseline |
| Shortest Job First | SJF | Não | Requer conhecer o burst antecipadamente |
| Shortest Remaining Time First | SRTF | Sim | Versão preemptiva do SJF |
| Round Robin | RR | Sim | Parâmetro: quantum (ex.: 2) |
| Prioridade | PRIO | Configurável | Definir política de *aging* para evitar starvation |

### Saídas obrigatórias

Para cada algoritmo, o programa deve gerar:

1. **Diagrama de Gantt** (em texto), exemplo:
```
| P1 | P1 | P2 | P1 | P3 | P4 | ...
0    1    2    4    5    ...
```
2. **Tabela de métricas por processo**: tempo de espera, tempo de turnaround, tempo de resposta.
3. **Métricas agregadas**: médias de espera e turnaround, throughput (processos concluídos por unidade de tempo).
4. **Gráfico comparativo** entre os algoritmos (pode ser gerado com script auxiliar em Python/matplotlib, ou ferramenta de planilha).

### Requisitos técnicos
- Escalonador deve ser modular: cada algoritmo implementado como uma função/estratégia independente, permitindo fácil adição de novos algoritmos.
- Simulação orientada a eventos (avançar o tempo em incrementos ou saltar para o próximo evento relevante — evitar *busy loop* ingênuo quando possível).

### Estrutura de entrega
```
cpu-scheduler/
├── src/
│   ├── main.c
│   ├── scheduler_fcfs.c
│   ├── scheduler_sjf.c
│   ├── scheduler_rr.c
│   ├── scheduler_prio.c
│   └── metrics.c
├── entradas/
│   └── carga1.txt, carga2.txt, ...
├── saidas/
├── analise.py          # opcional, para gráficos
└── RELATORIO.pdf
```

### Critérios de avaliação
| Critério | Peso |
|---|---|
| Corretude de cada algoritmo | 50% (10% cada) |
| Cálculo correto das métricas | 20% |
| Qualidade da análise comparativa | 20% |
| Organização e modularidade do código | 10% |

### Perguntas para o relatório
- Qual algoritmo minimizou o tempo médio de espera para cada carga de teste? Por quê?
- Como o tamanho do quantum no Round Robin afeta o tempo de resposta vs. overhead de troca de contexto?
- Em que cenário o SJF pode causar *starvation*? Como mitigar isso com aging?

---

## 3. Simulador de Paginação

### Objetivo
Simular o comportamento de um gerenciador de memória virtual paginada, comparando algoritmos de substituição de página quanto ao número de *page faults*.

### Conceitos exercitados
- Tradução de endereços virtuais em páginas
- Memória física limitada (quadros/*frames*)
- Algoritmos de substituição de página
- Anomalia de Belady

### Entrada

Sequência de referências a páginas (uma por linha ou separadas por espaço):
```
7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
```
Mais um parâmetro: número de *frames* disponíveis na memória física (ex.: 3, 4, 5...).

### Algoritmos a implementar

| Algoritmo | Descrição |
|---|---|
| FIFO | Substitui a página mais antiga carregada |
| LRU (*Least Recently Used*) | Substitui a página menos recentemente usada |
| Ótimo (*Belady*) | Substitui a página que será usada mais tarde no futuro (limite teórico, exige conhecer toda a sequência) |
| Segunda Chance | Variante do FIFO com bit de referência |
| LFU (desejável) | Substitui a página menos frequentemente usada |

### Saídas obrigatórias

1. Para cada algoritmo e cada tamanho de memória testado:
   - Número total de *page faults*
   - Traço de execução mostrando o estado dos *frames* a cada referência, exemplo:

```
Ref: 7  Frames: [7,-,-]   FAULT
Ref: 0  Frames: [7,0,-]   FAULT
Ref: 1  Frames: [7,0,1]   FAULT
Ref: 2  Frames: [2,0,1]   FAULT (substitui 7 - FIFO)
...
```

2. **Gráfico**: número de *frames* (eixo X) vs. número de *page faults* (eixo Y), para todos os algoritmos — permite observar a Anomalia de Belady no FIFO.

### Requisitos técnicos
- Implementação eficiente do LRU (ex.: usando contador de tempo lógico ou lista duplamente encadeada), não apenas busca linear ingênua (aceitável para fins didáticos, mas o aluno deve comentar a complexidade).
- Código estruturado para facilitar variar o número de frames sem reescrever lógica.

### Estrutura de entrega
```
paging-simulator/
├── src/
│   ├── main.c
│   ├── fifo.c
│   ├── lru.c
│   ├── optimal.c
│   └── second_chance.c
├── entradas/
│   └── sequencia1.txt, sequencia2.txt
├── analise.py
└── RELATORIO.pdf
```

### Critérios de avaliação
| Critério | Peso |
|---|---|
| Corretude de cada algoritmo | 60% (15% cada) |
| Contagem correta de page faults e traço de execução | 15% |
| Análise da Anomalia de Belady | 15% |
| Organização do código | 10% |

### Perguntas para o relatório
- Reproduza a Anomalia de Belady com uma sequência específica. Explique por que ela ocorre no FIFO e por que não ocorre no LRU/Ótimo.
- Compare o desempenho do Segunda Chance com o FIFO puro. Em que casos há diferença?
- Por que o algoritmo Ótimo não é implementável em um sistema real?

---

## 4. Alocador de Memória Customizado

### Objetivo
Implementar um gerenciador de memória dinâmica (similar a `malloc`/`free`) operando sobre um heap simulado, comparando estratégias de alocação quanto à fragmentação e desempenho.

### Conceitos exercitados
- Gerência de heap e blocos livres/ocupados
- Fragmentação interna vs. externa
- Estratégias de alocação (First-Fit, Best-Fit, Worst-Fit)
- Coalescência (*merging*) de blocos livres adjacentes

### Especificação funcional

Implementar as funções:
```c
void  mem_init(size_t heap_size);
void* meu_malloc(size_t tamanho);
void  meu_free(void* ptr);
void  mem_stats(void);   // relatório de uso
void  mem_dump(void);    // visualização do heap (blocos livres/ocupados)
```

O "heap" deve ser um bloco de memória simulado — por exemplo, um array estático ou alocado uma única vez com `malloc` real no início do programa (`char heap[TAMANHO]`), sobre o qual toda a gerência é feita manualmente pelo aluno.

### Estrutura de metadados sugerida

Cada bloco (livre ou ocupado) deve conter um cabeçalho, por exemplo:
```c
typedef struct bloco {
    size_t tamanho;
    int livre;
    struct bloco* proximo;
    struct bloco* anterior;
} Bloco;
```

### Estratégias a implementar

| Estratégia | Regra de escolha do bloco livre |
|---|---|
| First-Fit | Primeiro bloco livre grande o suficiente |
| Best-Fit | Menor bloco livre que ainda comporte a requisição |
| Worst-Fit | Maior bloco livre disponível |

A estratégia deve ser selecionável via parâmetro de compilação ou variável global, para facilitar comparação.

### Requisitos obrigatórios
- Divisão de blocos (*splitting*) quando o bloco escolhido for maior que o necessário.
- Coalescência de blocos livres adjacentes ao liberar memória (evitar fragmentação externa crescente).
- Alinhamento de memória (ex.: múltiplos de 8 bytes).
- Tratamento de erros: `meu_free` de ponteiro inválido/duplicado (double-free), solicitação maior que o heap disponível.

### Testes obrigatórios (workloads)

O aluno deve gerar/executar pelo menos 3 padrões de teste:

1. **Alocações sequenciais** de tamanhos variados sem liberação — mede uso puro de espaço.
2. **Alocações e liberações intercaladas** (padrão aleatório) — mede fragmentação.
3. **Padrão adversarial**: alocar N blocos pequenos, liberar blocos alternados, tentar alocar um bloco grande — testa se a estratégia sofre fragmentação externa severa.

### Métricas a coletar
- Fragmentação interna total (espaço desperdiçado por alinhamento/arredondamento)
- Fragmentação externa (soma de espaços livres não utilizáveis por serem pequenos/isolados)
- Número de operações (comparações/iterações) por alocação — proxy de custo computacional
- Taxa de falha de alocação (quando não há bloco livre suficiente, mesmo havendo espaço total suficiente)

### Estrutura de entrega
```
mem-allocator/
├── src/
│   ├── alocador.c / alocador.h
│   └── testes.c
├── benchmarks/
│   └── workload1.c, workload2.c, workload3.c
├── resultados/
│   └── comparativo.csv
└── RELATORIO.pdf
```

### Critérios de avaliação
| Critério | Peso |
|---|---|
| Corretude da alocação/liberação (sem corromper heap) | 30% |
| Implementação das 3 estratégias | 30% |
| Splitting e coalescência funcionando | 15% |
| Análise comparativa de fragmentação | 15% |
| Tratamento de erros | 10% |

### Perguntas para o relatório
- Qual estratégia apresentou menor fragmentação externa no padrão adversarial? Isso confirma a teoria (Best-Fit tende a gerar muitos fragmentos pequenos)?
- Qual o custo, em número de operações, de cada estratégia à medida que o heap fica mais fragmentado?
- Como a coalescência de blocos afeta os resultados? Refaça um dos testes desativando-a e compare.

---

## 5. Problemas Clássicos de Sincronização

### Objetivo
Implementar soluções corretas para problemas clássicos de concorrência utilizando primitivas de sincronização (mutexes, semáforos, variáveis de condição), compreendendo condições de corrida, deadlock e starvation.

### Conceitos exercitados
- Threads POSIX (`pthreads`)
- Exclusão mútua (`pthread_mutex_t`)
- Semáforos (`sem_t`)
- Variáveis de condição (`pthread_cond_t`)
- Deadlock, *livelock* e *starvation*

### Problemas a implementar

#### 5.1 Produtor-Consumidor
- Buffer circular de tamanho finito (ex.: 10 posições).
- N threads produtoras e M threads consumidoras, configuráveis via linha de comando.
- Produtor bloqueia se buffer cheio; consumidor bloqueia se buffer vazio.
- Implementar com semáforos (`sem_full`, `sem_empty`, `mutex`).

#### 5.2 Jantar dos Filósofos
- 5 filósofos, 5 garfos (recursos compartilhados entre vizinhos).
- Cada filósofo alterna entre pensar, ficar com fome, pegar dois garfos, comer, devolver os garfos.
- **Deve implementar ao menos uma estratégia que evite deadlock**, por exemplo:
  - Ordenação de recursos (sempre pegar o garfo de menor índice primeiro)
  - Garçom/árbitro central (semáforo contando no máximo 4 filósofos sentados simultaneamente)
  - Hierarquia de recursos com timeout

#### 5.3 Leitores-Escritores
- Múltiplas threads leitoras podem acessar o recurso simultaneamente.
- Escritores precisam de acesso exclusivo (nenhum leitor ou outro escritor durante a escrita).
- Implementar a variante **com prioridade para escritores** (evita que escritores esperem indefinidamente enquanto leitores chegam continuamente).

### Requisito adicional: diagnóstico de bugs

Para **cada um dos três problemas**, o aluno deve entregar:

1. Uma versão **correta** e funcional.
2. Uma versão **com bug proposital** (ex.: ordem de `lock` trocada, semáforo inicializado com valor errado, esquecimento de `unlock` em um caminho de erro) que produza deadlock ou starvation observável.
3. Um pequeno texto explicando **como identificou o bug** (uso de logs, ferramentas como `gdb`, `strace`, ou inspeção manual da ordem de aquisição de locks) e a correção aplicada.

### Requisitos técnicos gerais
- Uso de `printf` com timestamps/IDs de thread para visualizar a interleaving durante testes (cuidado: `printf` não é atômico — usar um mutex de log se necessário).
- Programas devem aceitar parâmetros de configuração via linha de comando (número de threads, tamanho de buffer, tempo de simulação).
- Recomenda-se compilar com `-fsanitize=thread` (ThreadSanitizer) para detectar condições de corrida durante o desenvolvimento.

### Estrutura de entrega
```
sincronizacao/
├── produtor_consumidor/
│   ├── correto.c
│   └── com_bug.c
├── filosofos/
│   ├── correto.c
│   └── com_bug.c
├── leitores_escritores/
│   ├── correto.c
│   └── com_bug.c
├── Makefile
└── RELATORIO.pdf
```

### Critérios de avaliação
| Critério | Peso |
|---|---|
| Produtor-Consumidor correto | 20% |
| Jantar dos Filósofos correto (sem deadlock) | 20% |
| Leitores-Escritores correto (com prioridade de escritor) | 20% |
| Versões com bug + diagnóstico documentado (3 problemas) | 25% |
| Uso de ferramentas de análise (TSan, gdb, etc.) | 15% |

### Perguntas para o relatório
- Qual estratégia você escolheu para evitar deadlock no Jantar dos Filósofos? Justifique por que ela garante ausência de deadlock (prove informalmente, ex.: por que a ordenação de recursos impede o ciclo de espera).
- No Leitores-Escritores, como sua solução garante que escritores não sofram starvation quando há um fluxo contínuo de leitores?
- Descreva um caso em que o bug proposital foi difícil de reproduzir de forma determinística. Por que bugs de concorrência costumam ser não determinísticos?

---

## Observações Gerais para Todos os Trabalhos

### Ambiente e compilação
- O uso de **Linux (nativo ou WSL) é obrigatório**, devido ao uso extensivo de chamadas POSIX. Trabalhos que não compilem/executem em ambiente Linux serão desconsiderados.
- Todos os projetos devem incluir `Makefile` funcional.

### Relatório — formato e ferramenta obrigatórios
- O relatório deve ser escrito **no Overleaf**, utilizando o **modelo oficial da SBC (Sociedade Brasileira de Computação)** para artigos/relatórios técnicos.
- Estrutura mínima esperada: Introdução, Fundamentação/Conceitos, Metodologia/Implementação, Resultados Experimentais, Discussão (respostas às perguntas indicadas em cada trabalho) e Conclusão.
- Extensão recomendada: 5–8 páginas no formato SBC.

### O que deve ser entregue
A entrega final de cada trabalho deve conter obrigatoriamente:
1. **Código-fonte completo** (repositório/pasta organizada conforme estrutura sugerida em cada projeto).
2. **Fontes `.tex` do relatório** (projeto Overleaf exportado, incluindo todos os arquivos auxiliares: `.tex`, imagens, bibliografia, etc.).
3. **PDF final do relatório**, gerado a partir do mesmo projeto Overleaf entregue.

> Relatórios entregues apenas em PDF, sem os fontes `.tex`, ou que não sigam o modelo SBC, serão desconsiderados.

### Prazos

| Data | Entrega |
|---|---|
| **08/09/2026** | Entrega do **projeto**: implementação completa, descrição da implementação, validação e experimentos realizados |
| **20/09/2026** | Entrega final do **trabalho completo**: código + relatório (fontes `.tex` e PDF, modelo SBC) |

### Integridade acadêmica

- **Todas as entregas serão submetidas a verificação de plágio** (comparação de código entre grupos/turmas via ferramentas como MOSS, além de comparação textual dos relatórios).
- **Plágio anula o trabalho.** Cópia total ou parcial de código ou de relatório de outro grupo, de trabalhos de turmas anteriores ou de fontes externas sem citação resulta em **nota zero** para todos os envolvidos, sem possibilidade de reentrega, e pode acarretar as sanções acadêmicas previstas no regimento do curso.
- **Uso de Inteligência Artificial (ex.: ChatGPT, Copilot, Claude e similares) para gerar a implementação do código é proibido e também anula o trabalho.** O código deve ser produto do próprio entendimento e esforço do grupo. Suspeita de geração por IA (estilo de código incompatível com o nível da turma, ausência de erros/iterações típicas de desenvolvimento humano, incapacidade de explicar trechos do código em eventual arguição oral, etc.) poderá levar à reprovação do trabalho.
- O uso de IA é permitido **apenas** para tirar dúvidas conceituais gerais (ex.: entender o que é uma variável de condição), nunca para gerar trechos de código, relatório ou solução do problema proposto. Em caso de dúvida sobre o que é permitido, o aluno deve consultar o professor **antes** da entrega.
- Grupos poderão ser convocados para uma **arguição oral** sobre o código e o relatório entregues, a critério do professor, como forma de validar a autoria e o entendimento do trabalho.