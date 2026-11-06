"""
Análise dos resultados de resultados/comparativo.csv

Métricas principais: fragmentação interna, fragmentação externa, operações por
alocação, taxa de falha e tempo de execução.

Gera em resultados/: resumo.csv (tabela do TSO, a única tabela salva; as outras só aparecem no terminal), workload2_frag_externa.png, coalescencia.png, escala.png,
barra_heap.png, tempo_malloc.png e min_dados.png, além das tabelas impressas no terminal.

Workloads:
- workload1: alocações sequenciais de tamanhos variados, sem liberação (mede uso puro de espaço e fragmentação interna)
- workload2: alocações e liberações intercaladas, pseudoaleatórias com semente fixa (mede fragmentação externa ao longo do tempo)
- workload3: padrão adversarial: alocar N blocos pequenos, liberar blocos alternados e tentar alocar um bloco grande que caberia na soma dos buracos, mas não em nenhum deles isoladamente

alocador.c + estrategias.c + estatisticas.c -> testes.c + driver.c (verificação)

alocador.c + estrategias.c + estatisticas.c -> main.c (demo)

                                                workload1.c ┐
alocador.c + estrategias.c + estatisticas.c ->  workload2.c ├─> resultados/comparativo.csv ─> src/analise.py ─> tabelas + gráficos em resultados/ (experimentos)
                                                workload3.c ┘
"""

# python3 src/analise.py   (ou: make analise)

import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')  # sem janela: os gráficos são salvos em resultados/
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import numpy as np
import pandas as pd

if len(sys.argv) > 1: #o make analise passa o caminho do csv
    arq = Path(sys.argv[1])
else:
    arq = Path('resultados/comparativo.csv') #entrada dos arquivos
saida = arq.parent #saida dos arquivos: a mesma pasta do csv
ordem = ['first_fit', 'next_fit', 'best_fit', 'worst_fit']

def salvar(nome): #salvar os png na saida e não em janelas
    plt.tight_layout()
    plt.savefig(saida / nome, dpi=150)
    plt.close()


# ----------------- PREPARAÇÃO DOS DADOS E LEITURA -----------------
df = pd.read_csv(arq)

# cada execução (workload + estratégia + coalescência + heap) começa na operação 1
df['execucao'] = (df['operacao'] == 1).cumsum()

# ----------------- MÉTRICAS DERIVADAS (frag interna, frag externa e taxa de falha de alocação) -----------------
# Fragmentação externa = 1 - maior_livre / total_livre
df['frag_externa'] = np.where(df['total_livre'] > 0, 1 - (df['maior_livre'] / df['total_livre']), 0) #o total_livre precisa ser maior q zero (condição), ai vem a formula (true), senao o resultado é zero (false)

# n_malloc = alocacoes_ok + falhas: todas as chamadas de meu_malloc (igual ao mem_stats e ao roteiro)
df['n_malloc'] = df['alocacoes_ok'] + df['falhas']

# Operações (blocos examinados) por alocação = blocos_examinados / n_malloc
df['examinados_por_alocacao'] = np.where(df['n_malloc'] > 0, df['blocos_examinados'] / df['n_malloc'], np.nan) #o n_malloc precisa ser maior q zero (condicao), ai vem a formula (true), senao o resultado é nulo/Nan (false)

# os contadores (alocacoes_ok, falhas, blocos_examinados...) são acumulados a cada operação,
# então o total de uma execução é a ÚLTIMA linha dela (somar todas as linhas contaria repetido)
finais = df.groupby('execucao').tail(1).copy()
finais['frag_externa_media'] = finais['execucao'].map(df.groupby('execucao')['frag_externa'].mean()) #média da frag. externa ao longo da execução

# os gráficos de cada workload usam só as execuções com coalescência ligada
com_coal = df[df['coalescencia'] == 1]
finais_coal = finais[finais['coalescencia'] == 1]

# ---------------------- WORKLOAD 1 - ALOCAÇÕES SEQUENCIAS--------------------------
print('Workload 1 - alocações sequenciais')
# sem liberação, todas as estratégias alocam igual: só muda quantos blocos cada uma examina
# overhead = bytes gastos com cabeçalhos
print(finais_coal[finais_coal['workload'] == 'workload1'][['estrategia', 'alocacoes_ok', 'frag_interna', 'overhead']].to_string(index=False))

# -------------------- WORKLOAD 2 - OPERAÇÕES INTERCALADAS-----------------------
plt.figure(figsize=(10, 5))
for estrategia, grupo in com_coal[com_coal['workload'] == 'workload2'].groupby('estrategia'):
    grupo = grupo.sort_values('operacao')#ordena por eperação (bestfit, nextfit etc)
    plt.plot(grupo['operacao'], grupo['frag_externa'] * 100, label=estrategia)

plt.xlabel('Operação')
plt.ylabel('Fragmentação externa (%)')
plt.title('Workload 2 — Fragmentação externa ao longo das operações')
plt.legend()
plt.grid(True)
salvar('workload2_frag_externa.png')#ao inves de show()

# --------------------- WORKLOAD 3- PADRÃO ADVERSARIAL----------------------
print('\nWorkload 3 - padrão adversarial')
w3 = com_coal[com_coal['workload'] == 'workload3']

# situação do heap na primeira falha: o pedido grande que cabe na soma dos buracos, mas em nenhum deles
primeira_falha = w3[w3['falhas'] == 1].groupby('estrategia').head(1)
print(primeira_falha[['estrategia', 'operacao', 'falhas_frag', 'frag_externa', 'maior_livre', 'total_livre']].to_string(index=False))

# ------------------- COALESCÊNCIA LIGADA X DESLIGADA ----------------------
print('\nCoalescência ligada x desligada')
comp = finais[finais['workload'].isin(['workload2', 'workload3'])]
print(comp.pivot_table(index=['workload', 'estrategia'], columns='coalescencia', values=['falhas', 'frag_externa_media']))

fig, eixos = plt.subplots(1, 2, figsize=(12, 5))
w2c = comp[comp['workload'] == 'workload2'].pivot_table(index='estrategia', columns='coalescencia', values='frag_externa_media')
w3c = comp[comp['workload'] == 'workload3'].pivot_table(index='estrategia', columns='coalescencia', values='falhas')
(w2c * 100).plot(kind='bar', ax=eixos[0])
eixos[0].set_title('Workload 2 — Fragmentação externa média')
eixos[0].set_ylabel('Fragmentação externa (%)')
w3c.plot(kind='bar', ax=eixos[1])
eixos[1].set_title('Workload 3 — Falhas de alocação')
eixos[1].set_ylabel('Falhas')
for eixo in eixos:
    eixo.set_xlabel('Estratégia')
    eixo.tick_params(axis='x', rotation=0)
    eixo.set_ylim(top=eixo.get_ylim()[1] * 1.3) #espaço em cima para a legenda não cobrir as barras
    eixo.legend(['desligada', 'ligada'], title='Coalescência') #colunas 0 e 1 viram "desligada" e "ligada"
salvar('coalescencia.png')#ao inves de show()

# --------------------- ESCALA ------------------------
# execuções do workload 2 com heaps e números de operações diferentes, gravadas com workload = 'escala'
# aqui só interessa a última linha de cada execução: nela, operacao = total de operações
print('\nEscala - blocos examinados por alocação x tamanho do heap e número de operações')
esc = finais_coal[finais_coal['workload'] == 'escala']
print(esc.pivot_table(index=['heap_size', 'operacao'], columns='estrategia', values='examinados_por_alocacao'))

heaps = sorted(esc['heap_size'].unique())
valores = sorted(esc['operacao'].unique())
fig, eixos = plt.subplots(1, len(heaps), figsize=(5 * len(heaps), 4), sharey=True)
for eixo, heap in zip(eixos, heaps): #um gráfico para cada tamanho de heap
    for estrategia, grupo in esc[esc['heap_size'] == heap].groupby('estrategia'):
        eixo.plot(grupo['operacao'], grupo['examinados_por_alocacao'], marker='o', label=estrategia)
    eixo.set_xscale('log') #100, 1000, 10000 operações
    eixo.set_xticks(valores) #mostra os números de operações em vez de 10^2, 10^3...
    eixo.set_xticklabels(valores)
    eixo.minorticks_off()
    eixo.set_title(f'Heap de {heap} bytes')
    eixo.set_xlabel('Operações')
    eixo.grid(True)
eixos[0].set_ylabel('Blocos examinados por alocação')
eixos[0].legend()
salvar('escala.png')#ao inves de show()

# --------------------- TEMPO DE EXECUÇÃO------------------------
# usa as execuções normais: a escala só grava a primeira e a última operação de cada execução
sem_escala_linhas = com_coal[com_coal['workload'] != 'escala']

for coluna, nome in [('tempo_alloc_ns', 'alocação'), ('tempo_free_ns', 'liberação')]: #mesma conta para os dois tempos
    print(f'\nTempo de execução ({nome}): mediana (p50) e percentil 95 (p95), em ns')
    tempos = sem_escala_linhas.groupby('estrategia')[coluna] #median() e quantile() já ignoram os valores vazios (None)
    print(pd.DataFrame({'p50': tempos.median(), 'p95': tempos.quantile(0.95)}))

# -------------------------- BARRA DO HEAP -----------------------
# cada barra é o heap inteiro no fim da execução, com um retângulo para cada bloco
blocos = pd.read_csv(saida / 'blocos.csv').drop_duplicates() #rodar um workload de novo acrescenta linhas iguais

paineis = [ #(nome no csv, coalescência, título)
    ('workload1', 1, 'Workload 1: fim da execução'),
    ('workload2', 1, 'Workload 2: fim, coalescência ligada'),
    ('workload2', 0, 'Workload 2: fim, coalescência desligada'),
    ('workload3_falha', 1, 'Workload 3: no pedido grande que falha'),
    ('workload3', 1, 'Workload 3: fim, coalescência ligada'),
    ('workload3', 0, 'Workload 3: fim, coalescência desligada'),
]
VERDE = '#006300' #livre, como o TSO sugere (verde escuro: separa melhor do laranja no daltonismo)
LARANJA = '#eb6834' #ocupado

fig, eixos = plt.subplots(3, 2, figsize=(13, 8))
for eixo, (nome, coalescencia, titulo) in zip(eixos.flat, paineis):
    painel = blocos[(blocos['workload'] == nome) & (blocos['coalescencia'] == coalescencia)]
    for y, estrategia in enumerate(ordem):
        for _, bloco in painel[painel['estrategia'] == estrategia].iterrows():
            eixo.barh(y, bloco['bytes'], left=bloco['endereco'], height=0.7,
                      color=VERDE if bloco['livre'] else LARANJA,
                      hatch='///' if bloco['livre'] else None, #as listras ajudam em impressão preto e branco e no daltonismo
                      edgecolor='white', linewidth=0.8)
    eixo.set_yticks(range(len(ordem)))
    eixo.set_yticklabels(ordem)
    eixo.invert_yaxis() #first_fit em cima
    eixo.set_xlim(0, (painel['endereco'] + painel['bytes']).max())
    eixo.set_xlabel('Endereço no heap (bytes)')
    eixo.set_title(titulo, loc='left', fontsize=11)
legenda = [Patch(facecolor=VERDE, edgecolor='white', hatch='///', label='Livre'), Patch(facecolor=LARANJA, label='Ocupado')]
plt.tight_layout(rect=(0, 0.05, 1, 1)) #deixa espaço embaixo para a legenda
fig.legend(handles=legenda, loc='lower center', ncol=2, frameon=False)
plt.savefig(saida / 'barra_heap.png', dpi=150)
plt.close()

# -------------------------- COMPARAÇÃO COM O MALLOC REAL (opcional do TSO) -----------------------
print('\nTempo: nosso alocador x malloc real, mediana (p50) e percentil 95 (p95), em ns')
tempos = pd.read_csv(saida / 'tempo_malloc.csv').groupby(['operacao', 'quem'])['tempo_ns']
tabela_tempos = pd.DataFrame({'p50': tempos.median(), 'p95': tempos.quantile(0.95)}).reset_index()
print(tabela_tempos.to_string(index=False))

ordem_tempo = ['malloc_real'] + ordem
fig, eixos = plt.subplots(1, 2, figsize=(12, 4.5), sharey=True)
for eixo, operacao in zip(eixos, ['malloc', 'free']):
    linhas = tabela_tempos[tabela_tempos['operacao'] == operacao].set_index('quem').loc[ordem_tempo]
    linhas[['p50', 'p95']].plot(kind='barh', ax=eixo, color=['#2a78d6', '#eb6834'], legend=False)
    eixo.set_xlabel('Tempo por operação (ns)')
    eixo.set_ylabel('')
    eixo.set_title(operacao, loc='left')
eixos[0].invert_yaxis() #malloc_real em cima (os dois gráficos dividem o mesmo eixo y, então inverte uma vez só)
eixos[0].legend(frameon=False, loc='upper right')
salvar('tempo_malloc.png')

# -------------------------- EFEITO DO MIN_DADOS (pergunta 5 do relatório) -----------------------
# cada linha do csv é uma execução; a média das sementes evita depender de uma única sequência de pedidos
print('\nEfeito do MIN_DADOS: média de 30 sementes')
md = pd.read_csv(saida / 'min_dados.csv')
md['taxa_falha'] = 100 * md['falhas'] / (md['alocacoes_ok'] + md['falhas'])
md['frag_externa_pct'] = 100 * md['frag_externa_media']
medias_md = md.groupby(['min_dados', 'estrategia'])[['alocacoes_ok', 'taxa_falha', 'frag_interna_media', 'frag_externa_pct', 'n_livres_medio']].mean().reset_index()
print(medias_md.round(2).to_string(index=False))

metricas = [
    ('frag_interna_media', 'Fragmentação interna média (bytes)'),
    ('frag_externa_pct', 'Fragmentação externa média (%)'),
    ('taxa_falha', 'Taxa de falha (%)'),
]
marcas = ['o', 's', '^', 'D'] #as marcas diferentes ajudam no daltonismo
fig, eixos = plt.subplots(1, 3, figsize=(14, 4.2))
for eixo, (coluna, titulo) in zip(eixos, metricas):
    for estrategia, marca in zip(ordem, marcas):
        grupo = medias_md[medias_md['estrategia'] == estrategia]
        eixo.plot(grupo['min_dados'], grupo[coluna], marker=marca, label=estrategia)
    eixo.set_xticks(sorted(md['min_dados'].unique()))
    eixo.set_xlabel('MIN_DADOS (bytes)')
    eixo.set_title(titulo, loc='left', fontsize=11)
    eixo.set_ylim(bottom=0)
    eixo.grid(True)
eixos[0].legend()
salvar('min_dados.png')


# -------------------------- TABELONA FINAL -----------------------
# Estratégia | Aloc. OK | Falhas | Frag. ext. (%) | Maior livre | Exam./aloc., uma tabela por workload, coalescência ligada
print('\nTabela final resumida (modelo do TSO)')
resumo = finais_coal[finais_coal['workload'] != 'escala'][['workload', 'estrategia', 'alocacoes_ok', 'falhas', 'frag_externa_media', 'maior_livre', 'examinados_por_alocacao']].copy()
resumo['frag_externa_media'] = (100 * resumo['frag_externa_media']).round(1) #em %: média ao longo da execução
resumo['examinados_por_alocacao'] = resumo['examinados_por_alocacao'].round(2)
resumo.columns = ['workload', 'estrategia', 'aloc_ok', 'falhas', 'frag_ext_pct', 'maior_livre', 'exam_por_aloc'] #nomes curtos, como na tabela do TSO
print(resumo.to_string(index=False))

resumo.to_csv(saida / 'resumo.csv', index=False) # manda essa tabelona para um arquivo de planilha
