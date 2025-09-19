#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <chrono>
#include <semaphore.h>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <mutex>

bool is_prime(int n) {
    if (n < 2) {
        return false;
    }
    if (n == 2){
        return true;
    } 
    if (n % 2 == 0) {
        return false;
    }
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

class ProducerConsumer {
private:
    std::vector<int> buffer;          // Buffer compartilhado
    int buffer_size;                  // Tamanho do buffer (N)
    int total_to_consume;             // Total de números a consumir (M)
    int num_consumers;                // Número de threads consumidoras (Nc)

    sem_t empty_slots;                // Semáforo contador de slots vazios
    sem_t full_slots;                 // Semáforo contador de slots cheios
    sem_t mutex;                      // Semáforo binário para exclusão mútua

    std::atomic<int> write_index{0};  // Índice de escrita
    std::atomic<int> read_index{0};   // Índice de leitura
    std::atomic<int> consumed_count{0}; // Contador de números consumidos
    std::atomic<bool> should_stop{false}; // Flag para parar produtores
    std::atomic<int> current_occupancy{0}; // Ocupação atual do buffer

    // Para registro de ocupação do buffer
    std::vector<std::pair<double, int>> occupancy_log; // (timestamp, ocupação)
    std::mutex log_mutex;
    std::chrono::high_resolution_clock::time_point start_time;

    // Gerador de números aleatórios thread-safe
    thread_local static std::mt19937 generator;
    thread_local static std::uniform_int_distribution<int> distribution;


public:
    ProducerConsumer(int n, int m)
        : buffer_size(n), total_to_consume(m) {
        buffer.resize(buffer_size);

        // Inicializa semáforos
        sem_init(&empty_slots, 0, buffer_size);  // N slots vazios inicialmente
        sem_init(&full_slots, 0, 0);             // 0 slots cheios inicialmente
        sem_init(&mutex, 0, 1);                  // Semáforo binário para exclusão mútua

        // Reserva espaço para log de ocupação
        occupancy_log.reserve(2 * total_to_consume);
    }

    ~ProducerConsumer() {
        sem_destroy(&empty_slots);
        sem_destroy(&full_slots);
        sem_destroy(&mutex);
    }

    void log_occupancy() {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(now - start_time).count();

        std::lock_guard<std::mutex> lock(log_mutex);
        occupancy_log.push_back({elapsed, current_occupancy.load()});
    }

    void producer(int id) {
        // Inicializa gerador de números aleatórios para esta thread
        generator.seed(std::chrono::steady_clock::now().time_since_epoch().count() + id);
        distribution = std::uniform_int_distribution<int>(1, 10000000);

        while (!should_stop.load()) {
            // Gera número aleatório
            int number = distribution(generator);

            // Aguarda por um slot vazio
            sem_wait(&empty_slots);

            // Verifica se deve parar (pode ter mudado enquanto esperava)
            if (should_stop.load()) {
                sem_post(&empty_slots); // Devolve o slot
                break;
            }

            // Região crítica - acesso exclusivo ao buffer
            sem_wait(&mutex);

            int idx = write_index.load();
            buffer[idx] = number;
            write_index = (idx + 1) % buffer_size;

            // Atualiza ocupação
            current_occupancy.fetch_add(1);
            log_occupancy();

            sem_post(&mutex);

            // Sinaliza que há um slot cheio
            sem_post(&full_slots);
        }
    }

    void consumer(int id) {
        while (consumed_count.load() < total_to_consume) {
            sem_wait(&full_slots);

            // Verifica se já consumiu o que deveria
            if (consumed_count.load() >= total_to_consume) {
                sem_post(&full_slots); 
                break;
            }

            sem_wait(&mutex); //mutex

            int idx = read_index.load();
            int number = buffer[idx];
            read_index = (idx + 1) % buffer_size;

            int current_consumed = consumed_count.fetch_add(1) + 1;

            current_occupancy.fetch_sub(1);
            log_occupancy();

            if (current_consumed >= total_to_consume) {
                should_stop.store(true);

                for (int i = 0; i < this->num_consumers; ++i) {
                    sem_post(&full_slots);
                }
            }

            sem_post(&mutex); //mutex

            bool prime = is_prime(number);

            sem_post(&empty_slots);
        }
    }

    void run(int np, int nc) {
        this->num_consumers = nc; 

        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        start_time = std::chrono::high_resolution_clock::now();

        // Cria threads produtoras
        for (int i = 0; i < np; i++) {
            producers.emplace_back(&ProducerConsumer::producer, this, i + 1);
        }

        // Cria threads consumidoras
        for (int i = 0; i < this->num_consumers; i++) {
            consumers.emplace_back(&ProducerConsumer::consumer, this, i + 1);
        }

        // Espera os consumidores terminarem
        for (auto& t : consumers) {
            t.join();
        }

        // Garante que produtores pararam
        should_stop.store(true);

        // Libera produtores que possam estar presos
        for (int i = 0; i < np; i++) {
            sem_post(&empty_slots);
        }

        // Espera threads produtoras terminarem
        for (auto& t : producers) {
            t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << duration.count() << std::endl;
    }

    void save_occupancy_log(const std::string& filename) {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto& entry : occupancy_log) {
                file << entry.first << "," << entry.second << "\n";
            }
            file.close();
        }
    }
};

thread_local std::mt19937 ProducerConsumer::generator;
thread_local std::uniform_int_distribution<int> ProducerConsumer::distribution;

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 6) {
        std::cerr << "Uso: " << argv[0] << " <N_buffer_size> <Np_producers> <Nc_consumers> [M_total] [verbose]" << std::endl;
        std::cerr << "Exemplo: " << argv[0] << " 10 2 4" << std::endl;
        std::cerr << "         " << argv[0] << " 10 2 4 1000" << std::endl;
        std::cerr << "         " << argv[0] << " 10 2 4 1000 1" << std::endl;
        std::cerr << "\nPara executar todos os casos de teste, use:" << std::endl;
        std::cerr << "  N ∈ {1, 10, 100, 1000}" << std::endl;
        std::cerr << "  (Np, Nc) ∈ {(1,1), (1,2), (1,4), (1,8), (2,1), (4,1), (8,1)}" << std::endl;
        return 1;
    }

    int N = std::atoi(argv[1]);   // Tamanho do buffer
    int Np = std::atoi(argv[2]);  // Número de produtores
    int Nc = std::atoi(argv[3]);  // Número de consumidores
    int M = (argc >= 5) ? std::atoi(argv[4]) : 100000;  // Total de números a consumir 10^5

    ProducerConsumer pc(N, M);
    pc.run(Np, Nc);

    if (argc >= 5) {
        std::string filename = "occupancy_N" + std::to_string(N) +
                              "_Np" + std::to_string(Np) +
                              "_Nc" + std::to_string(Nc) + ".csv";
        pc.save_occupancy_log(filename);
    }

    return 0;
}