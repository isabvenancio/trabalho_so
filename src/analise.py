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

from pathlib import Path
import matplotlib
matplotlib.use('Agg')  # sem janela: os gráficos são salvos em resultados/
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

raiz = Path(__file__).resolve().parent.parent
arq = raiz / 'resultados' / 'comparativo.csv' #entrada dos arquivos
saida = raiz / 'resultados' #saida dos arquivos

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

col_numericas = ['heap_size', 'operacoes', 'alocacoes_ok', 'falhas', 'maior_livre', 'total_livre', 'frag_interna', 'blocos_examinados', 'tempo_alloc_ns', 'tempo_free_ns']
for coluna in col_numericas:
    if coluna in df.columns:
        df[coluna] = pd.to_numeric(df[coluna], errors='coerce')

df.head()

# ----------------- MÉTRICAS DERIVADAS (frag interna, frag externa e taxa de falha de alocação) -----------------
# Fragmentação externa = 1 - maior_livre / total_livre
if {'maior_livre', 'total_livre'}.issubset(df.columns):
    df['frag_externa'] = np.where(df['total_livre'] > 0, 1 - (df['maior_livre'] / df['total_livre']), 0) #o total_livre precisa ser maior q zero (condição), ai vem a formula (true), senao o resultado é zero (false)

# Operações (blocos examinados) por alocação = blocos_examinados / alocacoes_ok
if {'blocos_examinados', 'alocacoes_ok'}.issubset(df.columns):
    df['examinados_por_alocacao'] = np.where(df['alocacoes_ok'] > 0, df['blocos_examinados'] / df['alocacoes_ok'], np.nan) #o alocacoes_ok precisa ser maior q zero (condicao), ai vem a formula (true), senao o resultado é nulo/Nan (false)

# Taxa de falha da alocação (%) = (falhas / operacoes) * 100
if {'falhas', 'operacoes'}.issubset(df.columns):
    df['taxa_falha'] = np.where(df['operacoes'] > 0, 100 * df['falhas'] / df['operacoes'], 0) #operacoes precisa ser maior q zero (condicao), ai vem a formula (true), senao o resultado da taxa é 0% (false)

print('Métricas derivadas')
print(df.head())

print('Resumo por workload e estratégia')
print(df.groupby(['workload', 'estrategia']).agg({ #groupby com função sum e mean
    'alocacoes_ok': 'sum', #soma
    'falhas': 'sum',
    'frag_externa': 'mean', #media
    'examinados_por_alocacao': 'mean',
}).reset_index())

# ---------------------- WORKLOAD 1 - ALOCAÇÕES SEQUENCIAS--------------------------
print('Workload 1 - alocações sequenciais')
w1 = df[df['workload'] == 'workload1'].copy()
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
w2 = df[df['workload'] == 'workload2'].copy()

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
w3 = df[df['workload'] == 'workload3'].copy()

print(w3.groupby('estrategia').agg({ #groupby com mean, maximo e minimo
    'falhas': 'sum', 
    'frag_externa': 'max', 
    'maior_livre': 'min'
}).reset_index())

# ------------------- NUMERO DE ALOCAÇÕES POR OPERAÇÃO---------------------- 
print('Número de operações por alocação')
custo = df.groupby('estrategia')['examinados_por_alocacao'].mean().sort_values()
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
resumo = (df.groupby('estrategia').agg({
    'alocacoes_ok': 'sum',
    'falhas': 'sum',
    'frag_externa': 'mean',
    'examinados_por_alocacao': 'mean',
}).reset_index())
print(resumo)

resumo.to_csv(saida / 'resumo.csv', index=False) # manda essa tabelona para um arquivo de planilha
