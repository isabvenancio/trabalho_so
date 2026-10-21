"""
Análise dos resultados de resultados/comparativo.csv

Métricas principais: fragmentação interna, fragmentação externa, operações por
alocação, taxa de falha e tempo de execução.

Workloads:
- workload1: alocações sequenciais de tamanhos variados, sem liberação (mede uso puro de espaço e fragmentação interna)
- workload2: alocações e liberações intercaladas, pseudoaleatórias com semente fixa (mede fragmentação externa ao longo do tempo)
- workload3: padrão adversarial: alocar N blocos pequenos, liberar blocos alternados e tentar alocar um bloco grande que caberia na soma dos buracos, mas não em nenhum deles isoladamente

workload1.c ┐
workload2.c ├─> alocador (meu_malloc / meu_free) ─> resultados/comparativo.csv ─> src/analise.py ─> tabelas + gráficos em resultados/
workload3.c ┘
"""

# python3 src/analise.py resultados/teste/comparativo.csv

import sys
from pathlib import Path
import matplotlib
matplotlib.use('Agg')  # sem janela: os gráficos são salvos em resultados/
import matplotlib.pyplot as plt
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
    'frag_interna': 'max'
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
w3 = finais_coal[finais_coal['workload'] == 'workload3'].copy()

print(w3.groupby('estrategia').agg({ #groupby com mean, maximo e minimo
    'falhas': 'sum',
    'frag_externa': 'max',
    'maior_livre': 'min'
}).reset_index())

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
print('Escala - blocos examinados por alocação x tamanho do heap e número de operações')
esc = finais_coal[finais_coal['workload'] == 'escala']
if len(esc) > 0:
    print(esc.pivot_table(index=['heap_size', 'operacoes'], columns='estrategia', values='examinados_por_alocacao'))

    heaps = sorted(esc['heap_size'].unique())
    fig, eixos = plt.subplots(1, len(heaps), figsize=(5 * len(heaps), 4), sharey=True, squeeze=False)
    for eixo, heap in zip(eixos[0], heaps): #um gráfico para cada tamanho de heap
        for estrategia, grupo in esc[esc['heap_size'] == heap].groupby('estrategia'):
            grupo = grupo.sort_values('operacoes')
            eixo.plot(grupo['operacoes'], grupo['examinados_por_alocacao'], marker='o', label=estrategia)
        eixo.set_xscale('log') #100, 1000, 10000 operações
        valores = sorted(esc['operacoes'].unique())
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
if 'tempo_alloc_ns' in df.columns:
    print('Tempo de execução (alocação): mediana (p50) e percentil 95 (p95)')
    print(df.groupby('estrategia')['tempo_alloc_ns'].agg(
        p50=lambda x: np.percentile(x.dropna(), 50), #dropna com lambda é pra tirar os valores com None, ao inves de ficar renomeando
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
