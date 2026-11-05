"""
Análise dos resultados de resultados/comparativo.csv

Métricas principais: fragmentação interna, fragmentação externa, operações por
alocação, taxa de falha e tempo de execução.

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

raiz = Path(__file__).resolve().parent.parent
if len(sys.argv) > 1: #o make analise passa o caminho do csv
    arq = Path(sys.argv[1])
else:
    arq = raiz / 'resultados' / 'comparativo.csv' #entrada dos arquivos
saida = arq.parent #saida dos arquivos: a mesma pasta do csv

def salvar(nome): #salvar os png na saida e não em janelas
    plt.tight_layout()
    plt.savefig(saida / nome, dpi=150)
    plt.close()


# ----------------- PREPARAÇÃO DOS DADOS E LEITURA -----------------
df = pd.read_csv(arq)

df.head()
print('Dados')
print('Linhas:', len(df))
print('Colunas:', df.columns.tolist())

df.info()

df.shape

col_numericas = ['operacao', 'heap_size', 'operacoes', 'alocacoes_ok', 'falhas', 'falhas_frag', 'coalescencia', 'maior_livre', 'total_livre', 'frag_interna', 'blocos_examinados', 'tempo_alloc_ns', 'tempo_free_ns']
for coluna in col_numericas:
    if coluna in df.columns:
        df[coluna] = pd.to_numeric(df[coluna], errors='coerce')

if 'coalescencia' not in df.columns: #csv antigo, sem a coluna: todas as execuções tinham coalescência ligada
    df['coalescencia'] = 1

# cada execução (workload + estratégia + coalescência + heap) começa na operação 1
df['execucao'] = (df['operacao'] == 1).cumsum()

df.head()

# ----------------- MÉTRICAS DERIVADAS (frag interna, frag externa e taxa de falha de alocação) -----------------
# Fragmentação externa = 1 - maior_livre / total_livre
if {'maior_livre', 'total_livre'}.issubset(df.columns):
    df['frag_externa'] = np.where(df['total_livre'] > 0, 1 - (df['maior_livre'] / df['total_livre']), 0) #o total_livre precisa ser maior q zero (condição), ai vem a formula (true), senao o resultado é zero (false)

# n_malloc = alocacoes_ok + falhas: todas as chamadas de meu_malloc (igual ao mem_stats e ao roteiro)
df['n_malloc'] = df['alocacoes_ok'] + df['falhas']

# Operações (blocos examinados) por alocação = blocos_examinados / n_malloc
if {'blocos_examinados', 'n_malloc'}.issubset(df.columns):
    df['examinados_por_alocacao'] = np.where(df['n_malloc'] > 0, df['blocos_examinados'] / df['n_malloc'], np.nan) #o n_malloc precisa ser maior q zero (condicao), ai vem a formula (true), senao o resultado é nulo/Nan (false)

# Taxa de falha da alocação (%) = (falhas / n_malloc) * 100
if {'falhas', 'n_malloc'}.issubset(df.columns):
    df['taxa_falha'] = np.where(df['n_malloc'] > 0, 100 * df['falhas'] / df['n_malloc'], 0) #n_malloc precisa ser maior q zero (condicao), ai vem a formula (true), senao o resultado da taxa é 0% (false)

print('Métricas derivadas')
print(df.head())

# os contadores (alocacoes_ok, falhas, blocos_examinados...) são acumulados a cada operação,
# então o total de uma execução é a ÚLTIMA linha dela (somar todas as linhas contaria repetido)
frag_media = df.groupby('execucao')['frag_externa'].mean() #média da frag. externa ao longo da execução
finais = df.groupby('execucao').tail(1).set_index('execucao')
finais['frag_externa_media'] = frag_media
finais = finais.reset_index()

# os gráficos de cada workload usam só as execuções com coalescência ligada
com_coal = df[df['coalescencia'] == 1]
finais_coal = finais[finais['coalescencia'] == 1]

print('Resumo por workload e estratégia')
print(finais.groupby(['workload', 'estrategia', 'coalescencia']).agg({ #groupby com função sum e mean
    'alocacoes_ok': 'sum', #soma
    'falhas': 'sum',
    'frag_externa_media': 'mean', #media
    'examinados_por_alocacao': 'mean',
}).reset_index())

# ---------------------- WORKLOAD 1 - ALOCAÇÕES SEQUENCIAS--------------------------
print('Workload 1 - alocações sequenciais')
w1 = finais_coal[finais_coal['workload'] == 'workload1'].copy()
print(w1.groupby('estrategia').agg({ #groupby com funcao sum e maximo
    'alocacoes_ok': 'sum',
    'frag_interna': 'max',
    'overhead': 'max' #bytes gastos com cabeçalhos
}).reset_index())

plt.figure(figsize=(8, 5))
for estrategia, grupo in w1.groupby('estrategia'):
    plt.bar(estrategia, grupo['frag_interna'].iloc[-1])
plt.xlabel('Estratégia')
plt.ylabel('Fragmentação interna (bytes)')
plt.title('Workload 1 — Fragmentação interna')
salvar('workload1_frag_interna.png') #ao inves de show()

# -------------------- WORKLOAD 2 - OPERAÇÕES INTERCALADAS----------------------- 
print('Workload 2 - operações intercaladas')
w2 = com_coal[com_coal['workload'] == 'workload2'].copy()

plt.figure(figsize=(10, 5))
for estrategia, grupo in w2.groupby('estrategia'):
    grupo = grupo.sort_values('operacao')#ordena por eperação (bestfit, nextfit etc)
    plt.plot(grupo['operacao'], grupo['frag_externa'] * 100, label=estrategia)

plt.xlabel('Operação')
plt.ylabel('Fragmentação externa (%)')
plt.title('Workload 2 — Fragmentação externa ao longo das operações')
plt.legend()
plt.grid(True)
salvar('workload2_frag_externa.png')#ao inves de show()

# --------------------- WORKLOAD 3- PADRÃO ADVERSARIAL---------------------- 
print('Workload 3 - padrão adversarial')
w3 = com_coal[com_coal['workload'] == 'workload3'].copy()

# situação do heap na primeira falha: o pedido grande que cabe na soma dos buracos, mas em nenhum deles
primeira_falha = w3[w3['falhas'] == 1].groupby('estrategia').head(1)
print(primeira_falha[['estrategia', 'operacao', 'falhas_frag', 'frag_externa', 'maior_livre', 'total_livre']])

# ------------------- COALESCÊNCIA LIGADA X DESLIGADA ----------------------
print('Coalescência ligada x desligada')
comp = finais[finais['workload'].isin(['workload2', 'workload3'])]
if comp['coalescencia'].nunique() > 1: #só faz sentido se o csv tem execuções com e sem coalescência
    print(comp.pivot_table(index=['workload', 'estrategia'], columns='coalescencia',
                           values=['falhas', 'frag_externa_media'], aggfunc='mean'))

    fig, eixos = plt.subplots(1, 2, figsize=(12, 5))
    w2c = comp[comp['workload'] == 'workload2'].pivot_table(index='estrategia', columns='coalescencia', values='frag_externa_media')
    w3c = comp[comp['workload'] == 'workload3'].pivot_table(index='estrategia', columns='coalescencia', values='falhas')
    if not w2c.empty:
        (w2c * 100).plot(kind='bar', ax=eixos[0])
    eixos[0].set_title('Workload 2 — Fragmentação externa média')
    eixos[0].set_ylabel('Fragmentação externa (%)')
    if not w3c.empty:
        w3c.plot(kind='bar', ax=eixos[1])
    eixos[1].set_title('Workload 3 — Falhas de alocação')
    eixos[1].set_ylabel('Falhas')
    for eixo in eixos:
        eixo.set_xlabel('Estratégia')
        eixo.tick_params(axis='x', rotation=0)
        eixo.set_ylim(top=eixo.get_ylim()[1] * 1.3) #espaço em cima para a legenda não cobrir as barras
        if eixo.get_legend() is not None: #colunas 0 e 1 viram "desligada" e "ligada"
            eixo.legend(['desligada', 'ligada'], title='Coalescência')
    salvar('coalescencia.png')#ao inves de show()
else:
    print('(o csv não tem execuções com e sem coalescência)')

# --------------------- ESCALA ------------------------
# execuções do workload 2 com heaps e números de operações diferentes, gravadas com workload = 'escala'
# aqui só interessa a última linha de cada execução: nela, operacao = total de operações
print('Escala - blocos examinados por alocação x tamanho do heap e número de operações')
esc = finais_coal[finais_coal['workload'] == 'escala']
if len(esc) > 0:
    print(esc.pivot_table(index=['heap_size', 'operacao'], columns='estrategia', values='examinados_por_alocacao'))

    heaps = sorted(esc['heap_size'].unique())
    fig, eixos = plt.subplots(1, len(heaps), figsize=(5 * len(heaps), 4), sharey=True, squeeze=False)
    for eixo, heap in zip(eixos[0], heaps): #um gráfico para cada tamanho de heap
        for estrategia, grupo in esc[esc['heap_size'] == heap].groupby('estrategia'):
            grupo = grupo.sort_values('operacao')
            eixo.plot(grupo['operacao'], grupo['examinados_por_alocacao'], marker='o', label=estrategia)
        eixo.set_xscale('log') #100, 1000, 10000 operações
        valores = sorted(esc['operacao'].unique())
        eixo.set_xticks(valores) #mostra os números de operações em vez de 10^2, 10^3...
        eixo.set_xticklabels([str(int(v)) for v in valores])
        eixo.minorticks_off()
        eixo.set_title(f'Heap de {heap} bytes')
        eixo.set_xlabel('Operações')
        eixo.grid(True)
    eixos[0][0].set_ylabel('Blocos examinados por alocação')
    eixos[0][0].legend()
    salvar('escala.png')#ao inves de show()
else:
    print('(o csv não tem execuções de escala)')

# ------------------- NUMERO DE ALOCAÇÕES POR OPERAÇÃO----------------------
print('Número de operações por alocação')
sem_escala = finais_coal[finais_coal['workload'] != 'escala']
custo = sem_escala.groupby('estrategia')['examinados_por_alocacao'].mean().sort_values()
print(custo)

plt.figure(figsize=(8, 5))
plt.bar(custo.index, custo.values)
plt.xlabel('Estratégia')
plt.ylabel('Blocos examinados por alocação')
plt.title('Custo médio da busca')
salvar('custo_busca.png')#ao inves de show()

# --------------------- TEMPO DE EXECUÇÃO------------------------ 
# usa as execuções normais: a escala só grava a primeira e a última operação de cada execução
sem_escala_linhas = com_coal[com_coal['workload'] != 'escala']

if 'tempo_alloc_ns' in df.columns:
    print('Tempo de execução (alocação): mediana (p50) e percentil 95 (p95)')
    print(sem_escala_linhas.groupby('estrategia')['tempo_alloc_ns'].agg(
        p50=lambda x: np.percentile(x.dropna(), 50), #dropna com lambda é pra tirar os valores com None, ao inves de ficar renomeando
        p95=lambda x: np.percentile(x.dropna(), 95),
    ).reset_index())

if 'tempo_free_ns' in df.columns:
    print('Tempo de execução (liberação): mediana (p50) e percentil 95 (p95)')
    print(sem_escala_linhas.groupby('estrategia')['tempo_free_ns'].agg(
        p50=lambda x: np.percentile(x.dropna(), 50),
        p95=lambda x: np.percentile(x.dropna(), 95),
    ).reset_index())

# -------------------------- TABELONA FINAL----------------------- 
print('Tabela final resumida')
resumo = (sem_escala.groupby('estrategia').agg({
    'alocacoes_ok': 'sum',
    'falhas': 'sum',
    'frag_externa_media': 'mean',
    'examinados_por_alocacao': 'mean',
}).reset_index())
print(resumo)

resumo.to_csv(saida / 'resumo.csv', index=False) # manda essa tabelona para um arquivo de planilha

# -------------------------- BARRA DO HEAP (opcional do TSO) -----------------------
# cada barra é o heap inteiro no fim da execução, com um retângulo para cada bloco
print('Barra do heap')
blocos = pd.read_csv(saida / 'blocos.csv').drop_duplicates() #rodar um workload de novo acrescenta linhas iguais

ordem = ['first_fit', 'next_fit', 'best_fit', 'worst_fit']
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
    eixo.set_xlim(0, painel['endereco'].add(painel['bytes']).max())
    eixo.set_xlabel('Endereço no heap (bytes)')
    eixo.set_title(titulo, loc='left', fontsize=11)
    eixo.spines[['top', 'right', 'left']].set_visible(False)
    eixo.tick_params(axis='y', length=0)
legenda = [Patch(facecolor=VERDE, edgecolor='white', hatch='///', label='Livre'), Patch(facecolor=LARANJA, label='Ocupado')]
plt.tight_layout(rect=(0, 0.05, 1, 1)) #deixa espaço embaixo para a legenda
fig.legend(handles=legenda, loc='lower center', ncol=2, frameon=False)
plt.savefig(saida / 'barra_heap.png', dpi=150)
plt.close()

# -------------------------- COMPARAÇÃO COM O MALLOC REAL (opcional do TSO) -----------------------
print('Tempo: nosso alocador x malloc real, mediana (p50) e percentil 95 (p95), em ns')
tempos = pd.read_csv(saida / 'tempo_malloc.csv')
tabela_tempos = tempos.groupby(['operacao', 'quem'])['tempo_ns'].agg(
    p50=lambda x: np.percentile(x, 50),
    p95=lambda x: np.percentile(x, 95),
).reset_index()
print(tabela_tempos)
tabela_tempos.to_csv(saida / 'resumo_tempo_malloc.csv', index=False)

ordem_tempo = ['malloc_real'] + ordem
fig, eixos = plt.subplots(1, 2, figsize=(12, 4.5), sharey=True)
for eixo, operacao in zip(eixos, ['malloc', 'free']):
    linhas = tabela_tempos[tabela_tempos['operacao'] == operacao].set_index('quem').loc[ordem_tempo]
    for deslocamento, coluna, cor in [(-0.2, 'p50', '#2a78d6'), (0.2, 'p95', '#eb6834')]:
        barras = eixo.barh(np.arange(len(ordem_tempo)) + deslocamento, linhas[coluna], height=0.36, color=cor, label=coluna)
        eixo.bar_label(barras, fmt='%.0f', padding=3, fontsize=9)
    eixo.set_yticks(range(len(ordem_tempo)))
    eixo.set_yticklabels(ordem_tempo)
    eixo.set_xlabel('Tempo por operação (ns)')
    eixo.set_title(operacao, loc='left')
    eixo.spines[['top', 'right']].set_visible(False)
    eixo.margins(x=0.12) #espaço para os números na ponta das barras
eixos[0].invert_yaxis() #malloc_real em cima (os dois gráficos dividem o mesmo eixo y, então inverte uma vez só)
eixos[0].legend(frameon=False, loc='upper right')
salvar('tempo_malloc.png')

# -------------------------- EFEITO DO MIN_DADOS (pergunta 5 do relatório) -----------------------
# cada linha do csv é uma execução; a média das sementes evita depender de uma única sequência de pedidos
print('Efeito do MIN_DADOS: média de 30 sementes')
md = pd.read_csv(saida / 'min_dados.csv')
md['taxa_falha'] = 100 * md['falhas'] / (md['alocacoes_ok'] + md['falhas'])
md['frag_externa_pct'] = 100 * md['frag_externa_media']
medias_md = md.groupby(['min_dados', 'estrategia'])[['alocacoes_ok', 'taxa_falha', 'frag_interna_media', 'frag_externa_pct', 'n_livres_medio']].mean().reset_index()
print(medias_md.round(2))
medias_md.to_csv(saida / 'resumo_min_dados.csv', index=False)

metricas = [
    ('frag_interna_media', 'Fragmentação interna média (bytes)'),
    ('frag_externa_pct', 'Fragmentação externa média (%)'),
    ('taxa_falha', 'Taxa de falha (%)'),
]
estilos = {'first_fit': ('#2a78d6', 'o'), 'next_fit': ('#eb6834', 's'), 'best_fit': ('#1baf7a', '^'), 'worst_fit': ('#eda100', 'D')} #as marcas diferentes ajudam no daltonismo
fig, eixos = plt.subplots(1, 3, figsize=(14, 4.2))
for eixo, (coluna, titulo) in zip(eixos, metricas):
    for estrategia in ordem:
        grupo = medias_md[medias_md['estrategia'] == estrategia]
        cor, marca = estilos[estrategia]
        eixo.plot(grupo['min_dados'], grupo[coluna], color=cor, marker=marca, markersize=7, linewidth=2, label=estrategia)
    eixo.set_xticks(sorted(md['min_dados'].unique()))
    eixo.set_xlabel('MIN_DADOS (bytes)')
    eixo.set_title(titulo, loc='left', fontsize=11)
    eixo.set_ylim(bottom=0)
    eixo.grid(True, axis='y', color='#dddddd')
    eixo.spines[['top', 'right']].set_visible(False)
plt.tight_layout(rect=(0, 0.08, 1, 1)) #deixa espaço embaixo para a legenda
linhas, nomes = eixos[0].get_legend_handles_labels()
fig.legend(linhas, nomes, loc='lower center', ncol=4, frameon=False)
plt.savefig(saida / 'min_dados.png', dpi=150)
plt.close()
