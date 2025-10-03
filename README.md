## Requisitos

### Compilador C++

* **g++ 9.0+** com suporte a `-std=c++17` e pthreads.
* Linux/Mac: já incluso.
* Windows: usar WSL ou MinGW-w64.

### Python

* **Python 3.10+** (testado também em 3.11).
* Bibliotecas necessárias:

  ```bash
  pip install numpy pandas matplotlib
  ```

## Execução dos Programas

### 1. Versão Pipe (processos e fork)

Dentro da pasta `pipes/`:

```bash
cd pipes

# Compilar
g++ -std=c++17 pipe_main.cpp -o pipe_exec

# Executar (gera 1000 números)
./pipe_exec 1000
```

O produtor gera números crescentes aleatórios e o consumidor imprime indicando se são primos.

### 2. Versão Semáforo (threads + buffer compartilhado)

Dentro da pasta `semaforo/`:

```bash
cd semaforo

# Compilar
g++ -std=c++17 -O3 main.cpp -o producer_consumer -pthread

# Executar
./producer_consumer <N_buffer> <Np_produtores> <Nc_consumidores> [M_total]
```

Exemplo:

```bash
./producer_consumer 10 2 4 1000
```

* **N_buffer**: tamanho do buffer (1, 10, 100, 1000)
* **Np_produtores**: número de threads produtoras
* **Nc_consumidores**: número de threads consumidoras
* **M_total**: total de elementos a consumir (default: 100000)

### 3. Execução de Todos os Cenários (Python)

Ainda dentro da pasta `semaforo/`:

```bash
python3 main.py
```

O script:

* Compila o `main.cpp` automaticamente
* Executa todos os cenários:

  * N ∈ {1, 10, 100, 1000}
  * (Np, Nc) ∈ {(1,1), (1,2), (1,4), (1,8), (2,1), (4,1), (8,1)}
* Roda cada configuração 10 vezes para média
* Gera gráficos:

  * **performance_graph.png** (tempo médio de execução)
  * **occupancy_N*_Np*_Nc*.png** (ocupação do buffer ao longo do tempo)
* Os resultados ficam salvos na pasta `results/`.

## Análises Realizadas

### Grupo 1: Buffer Unitário (N=1)

* Buffer sempre cheio.
* Sistema sincronizado: produtor só insere quando vazio, consumidor só lê quando cheio.
* Performance ruim e não melhora com mais threads.

### Grupo 2: Buffer Pequeno (N=10)

* **Np=1, Nc=1**: buffer cheio quase sempre, estável.
* **Np=1, Nc=2-8**: consumidores pressionam o buffer, ocupação cai em torno de 5.
* **Np=2-8, Nc=1**: buffer quase sempre cheio, produtores bloqueados (desperdício de produção).

### Grupo 3: Buffer Médio (N=100)

* **Np=1, Nc=1**: alta variabilidade, bursts de produção/consumo.
* **Np=1, Nc=2-8**: consumidores dominam, buffer tende a níveis baixos.
* **Np=2-8, Nc=1**: buffer cheio quase sempre, consumidor é gargalo.

### Grupo 4: Buffer Grande (N=1000)

* **Np=1, Nc=1**: buffer quase cheio (>950).
* **Np=1, Nc=2-8**: consumidores dominam, buffer quase vazio.
* **Np=2-8, Nc=1**: buffer cheio (>950), consumidores não acompanham.

## Conclusões e Recomendações

1. **Impacto do Buffer Unitário:** gargalo principal, tempos até 400ms.
2. **Buffers Maiores (10, 100, 1000):** tempos similares (65-90ms), buffer grande não traz ganho.
3. **Configurações Balanceadas (Np ≈ Nc):** melhor uso de recursos.
4. **Desequilíbrios (Np ≫ Nc ou Nc ≫ Np):** desperdício de threads e gargalos evidentes.

### Trade-offs

* **Memória vs Flexibilidade:** buffer de 10–100 é geralmente ótimo.
* **Paralelismo vs Contenção:** mais threads não significa mais desempenho.
* **Latência vs Throughput:** buffers pequenos = menor latência, buffers maiores = maior throughput.

## Considerações Finais

* Não existe solução única, depende da aplicação.
* Monitoramento da ocupação é essencial para identificar gargalos.
* O buffer por si só não resolve desequilíbrios entre produção e consumo.
* Sincronização tem custo, evitar excesso de threads bloqueadas.
