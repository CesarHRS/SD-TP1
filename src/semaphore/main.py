import os
import subprocess
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

CPP_SOURCE_FILE = "main.cpp"
EXECUTABLE_NAME = "./producer_consumer"
NUM_RUNS = 10  
M_VALUE = 100000 
RESULTS_DIR = "results" 

N_VALUES = [1, 10, 100, 1000]
THREAD_CONFIGS = [(1, 1), (1, 2), (1, 4), (1, 8), (2, 1), (4, 1), (8, 1)]

def compile_cpp():
    print(f"Compilando {CPP_SOURCE_FILE}...")
    try:
        subprocess.run(
            ["g++", "-std=c++17", "-O3", CPP_SOURCE_FILE, "-o", EXECUTABLE_NAME, "-pthread"],
            check=True,
            capture_output=True,
            text=True
        )
        print("Compilação bem-sucedida.")
        return True
    except FileNotFoundError:
        print("Erro: g++ não encontrado!")
        return False
    except subprocess.CalledProcessError as e:
        print("Erro durante a compilação:")
        print(e.stderr)
        return False

# Altere os parâmetros da função
def run_single_test(n, n_p, n_c):
    command = [EXECUTABLE_NAME, str(n), str(n_p), str(n_c), str(M_VALUE)]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return int(result.stdout.strip())

def plot_performance(results):
    fig, ax = plt.subplots(figsize=(12, 8))
    
    x_labels = [f"({p},{c})" for p, c in THREAD_CONFIGS]

    for n_val, timings in results.items():
        ax.plot(x_labels, timings, marker='o', linestyle='-', label=f'N = {n_val}')

    ax.set_xlabel("Configuração de Threads (Produtores, Consumidores)")
    ax.set_ylabel("Tempo Médio de Execução (ms)")
    ax.set_title("Desempenho do Produtor-Consumidor por Tamanho do Buffer (N)")
    ax.legend()
    ax.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()
    
    output_path = os.path.join(RESULTS_DIR, "performance_graph.png")
    plt.savefig(output_path)
    print(f"Gráfico de desempenho salvo em: {output_path}")
    plt.close(fig)

def plot_occupancy_all():
    print("Gerando gráficos de ocupação do buffer...")
    for n in N_VALUES:
        for n_p, n_c in THREAD_CONFIGS:
            filename = f"occupancy_N{n}_Np{n_p}_Nc{n_c}.csv"
            
            if not os.path.exists(filename):
                print(f"Aviso: Arquivo de log não encontrado: {filename}")
                continue
                
            try:
                df = pd.read_csv(filename, header=None, names=['timestamp', 'occupancy'])
                
                df['timestamp'] = df['timestamp'] * 1000
                
                fig, ax = plt.subplots(figsize=(10, 6))
                ax.plot(df['timestamp'], df['occupancy'], linestyle='-')
                
                ax.set_xlabel("Tempo (ms)")
                ax.set_ylabel("Ocupação do Buffer")
                ax.set_title(f"Ocupação do Buffer ao Longo do Tempo\n(N={n}, Np={n_p}, Nc={n_c})")
                ax.grid(True, linestyle='--', linewidth=0.5)
                ax.yaxis.set_major_locator(mticker.MaxNLocator(integer=True))
                ax.set_ylim(bottom=0, top=max(1, n))

                output_filename = f"occupancy_N{n}_Np{n_p}_Nc{n_c}.png"
                output_path = os.path.join(RESULTS_DIR, output_filename)
                plt.savefig(output_path)
                plt.close(fig)
                os.remove(filename)

            except Exception as e:
                print(f"Erro ao processar {filename}: {e}")

def main():
    # roda os testes e gera os gráficos
    if not compile_cpp():
        return
    if not os.path.exists(RESULTS_DIR):
        os.makedirs(RESULTS_DIR)

    performance_results = {n: [] for n in N_VALUES}

    for n_p, n_c in THREAD_CONFIGS:
        for n in N_VALUES:
            times = []
            print(f"Testando: N={n}, Np={n_p}, Nc={n_c} ...", end="", flush=True)
            for _ in range(NUM_RUNS):
                times.append(run_single_test(n, n_p, n_c))
            
            avg_time = np.mean(times)
            performance_results[n].append(avg_time)
            print(f" Tempo médio: {avg_time:.2f} ms")

    plot_performance(performance_results)
    plot_occupancy_all()
    print(f"Resultados salvos!")
    
if __name__ == "__main__":
    main()