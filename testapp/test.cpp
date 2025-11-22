#include <iostream>
#include <vector>
#include <chrono>
#include <random>

int main() {
    const size_t N = 1500; // размер матриц
    std::vector<std::vector<double>> A(N, std::vector<double>(N));
    std::vector<std::vector<double>> B(N, std::vector<double>(N));
    std::vector<std::vector<double>> C(N, std::vector<double>(N, 0.0));

    // Инициализация случайными числами
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < N; ++i)
        for (size_t j = 0; j < N; ++j) {
            A[i][j] = dist(gen);
            B[i][j] = dist(gen);
        }

    auto start = std::chrono::high_resolution_clock::now();

    // Классическое перемножение матриц
    std::cout << "Starting to calc matrices" << std::endl;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < N; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Time elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "C[0][0] = " << C[0][0] << std::endl; // чтобы компилятор не оптимизировал

    return 0;
}
