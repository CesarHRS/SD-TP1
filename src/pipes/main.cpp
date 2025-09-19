#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <quantidade_de_numeros>" << std::endl;
        return 1;
    }

    int quantidade = std::atoi(argv[1]);
    int fd[2];
    pipe(fd);

    if (fork() == 0) { // consumidor
        close(fd[1]); // fecha o produtor
        int num;
        while (true) {
            ssize_t bytes = read(fd[0], &num, sizeof(int));
            if (bytes <= 0) break;
            if (num == 0) break;
            std::cout << "Consumidor recebeu: " << num;
            if (is_prime(num))
                std::cout << " (primo)";
            else
                std::cout << " (não primo)";
            std::cout << std::endl;
        }
        close(fd[0]);
    } else { // Processo produtor
        close(fd[0]); // fecha o consumidor
        std::srand(std::time(nullptr));
        int N = 1;
        for (int i = 0; i < quantidade; ++i) {
            int delta = std::rand() % 100 + 1; // 1 - 100
            N += delta;
            write(fd[1], &N, sizeof(int));
        }
        int fim = 0;
        write(fd[1], &fim, sizeof(int));
        close(fd[1]);
    }
    return 0;
}