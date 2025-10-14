import numpy as np
import random
import os

def matriz_delays(d_matriz, n, d_intervalo, n_rest_prec, k):

    while(k < n_rest_prec):
        i = random.randint(0, n-1)
        j = random.randint(0, n-1)
        if(i != j and d_matriz[i][j] == 0 and d_matriz[j][i] == 0):
            d_matriz[i][j] = random.randint(d_intervalo[0], d_intervalo[1])
            d_matriz[j][i] = 0
            k+=1

    return d_matriz

def gerar_instancia_grafo(n, p_intervalo, s_intervalo, d_intervalo, n_rest_prec, seed=None):

    # n: número de jobs
    # p_intervalo: tupla (a, b) - intervalo do tempo de processamento
    # s_intervalo: tupla (x, y) - intervalo do setup time
    # d_intervalo: tupla (e, f) - intervalo do delay time
    # n_rest_prec: número de restrições de precedência
    # seed: seed para reproducibilidade
    
    if(n_rest_prec >= n):
        print("N° de restricoes incoerente")
        exit()


    if seed is not None:
        random.seed(seed)
        np.random.seed(seed)
    
    # Gerar tempos de processamento individuais para cada job
    p = [random.randint(p_intervalo[0], p_intervalo[1]) for _ in range(n)]
    
    # Matriz com tempos de setup
    s_matriz = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            if i != j:
                s_matriz[i][j] = random.randint(s_intervalo[0], s_intervalo[1])
    
    d_matriz = matriz_delays(np.zeros((n, n)), n, d_intervalo, n_rest_prec, 0)
        
    return {
        'n': n,
        'p': p,
        'x': n_rest_prec,
        's': s_matriz,
        'd': d_matriz
    }

def salvar_instancia(instancia, nome_arquivo):
    """Salva a instância em um arquivo de texto apenas com números"""
    with open(nome_arquivo, 'w') as f:
        # 1. Número de jobs
        f.write(f"{instancia['n']}\n")
        
        # 2. Tempos de processamento de cada job (um por linha)
        for tempo in instancia['p']:
            f.write(f"{tempo}\n")
        
        # 3. Matriz S (setup times) - uma linha por linha da matriz
        for i in range(instancia['n']):
            for j in range(instancia['n']):
                f.write(f"{int(instancia['s'][i][j])}")
                if j < instancia['n'] - 1:
                    f.write(" ")
            f.write("\n")
        
        # 4. Matriz D (delay times) - uma linha por linha da matriz
        for i in range(instancia['n']):
            for j in range(instancia['n']):
                f.write(f"{int(instancia['d'][i][j])}")
                if j < instancia['n'] - 1:
                    f.write(" ")
            f.write("\n")

def verificar_viabilidade(instancia):
    """Verifica se as condições de viabilidade foram atendidas"""
    n = instancia['n']
    d = instancia['d']
    
    # Condição 1: Se d[i][j] > 0 então d[j][i] = 0 e onde i = j, d[i][j] = 0
    condicao1_ok = True
    for i in range(n):
        for j in range(n):
            if i != j and d[i][j] > 0 and d[j][i] > 0:
                condicao1_ok = False
                print(f"ERRO Condição 1: d[{i}][{j}] = {d[i][j]} e d[{j}][{i}] = {d[j][i]}")
            if i==j:
                if d[i][j] != 0:
                    condicao1_ok = False
                    print(f"ERRO Condição 1: d[{i}][{j}] != 0")
    
    # Condição 2: Pelo menos um job com linha zerada (pode ser primeiro)
    condicao2_ok = False
    jobs_primeiros = []
    for i in range(n):
        linha_zerada = True
        for j in range(n):
            if i != j and d[i][j] > 0:
                linha_zerada = False
                break
        if linha_zerada:
            condicao2_ok = True
            jobs_primeiros.append(i)
    
    print(f"\nVerificação de viabilidade:")
    print(f"✓ Condição 1 (d[i][j] > 0 → d[j][i] = 0): {condicao1_ok}")
    print(f"✓ Condição 2 (pelo menos um job pode ser primeiro): {condicao2_ok}")
    if condicao2_ok:
        print(f"  Jobs que podem ser primeiro: {jobs_primeiros}")
    
    return condicao1_ok and condicao2_ok

def imprimir_instancia(instancia):

    print(f"Número de jobs: {instancia['n']}")
    print(f"Número de restrições: {instancia['x']}")
    print(f"Tempos de processamento: {instancia['p']}")
    
    print("\nMatriz S (setup times):")
    for i in range(instancia['n']):
        print(' '.join(f"{int(x):2d}" for x in instancia['s'][i]))
    
    print("\nMatriz D (delay times):")
    for i in range(instancia['n']):
        print(' '.join(f"{int(x):2d}" for x in instancia['d'][i]))
    
    # Verificar viabilidade
    verificar_viabilidade(instancia)

if __name__ == "__main__":

    # parametros
    n_jobs = 20
    n_restricoes_precedencia = 15
    p_range = (10, 20)    
    s_range = (5, 10)      
    d_range = (20, 40)      
    
    # Gerar instância
    instancia = gerar_instancia_grafo(n_jobs, p_range, s_range, d_range, n_restricoes_precedencia)#, seed=42)
    
    # Mostrar resultados
    imprimir_instancia(instancia)
    
    nome_instancia = "instancia5_grupo1.txt"

    # Salvar em arquivo
    #salvar_instancia(instancia, nome_instancia)
    #print(f"\nInstância salva em '{nome_instancia}'")

    print()
    print("Cara da instancia: \n")

    os.system(f'cat {nome_instancia}')
