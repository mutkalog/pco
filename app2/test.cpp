#include <iostream>
#include <vector>
#include <thread>
#include <cmath>
#include <chrono>
#include <atomic>

static std::atomic<bool> running{true};

// Загружает CPU реальными вычислениями (sin/cos) в несколько потоков
void cpu_stress() {
    double x = 0.0;
    while (running.load()) {
        for (int i = 0; i < 5'000'000; i++)
            x += std::sin(i) * std::cos(i * 0.5);
    }
    if (x == 0.123) std::cout << x; // чтобы не оптимизировал
}

// Постепенно выделяет память блоками и заполняет
void memory_stress(size_t total_mb, size_t step_mb) {
    size_t step_bytes = step_mb * 1024ULL * 1024ULL;
    size_t total_bytes = total_mb * 1024ULL * 1024ULL;

    std::vector<char*> blocks;
    blocks.reserve(total_mb / step_mb);

    size_t allocated = 0;

    while (allocated < total_bytes && running.load()) {
        char* block = new char[step_bytes];
        
        // Заполнить память – это создаёт настоящий memory pressure
        for (size_t i = 0; i < step_bytes; i += 4096)
            block[i] = static_cast<char>(i);

        blocks.push_back(block);
        allocated += step_bytes;

        std::cout << "Allocated: " << allocated / (1024*1024) << " MB\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // Чтобы процесс жил и использовал память
    while (running.load()) {
        for (auto& blk : blocks) {
            blk[0]++; // минимальное использование, чтобы cgroup не отправила в idle
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto blk : blocks) delete[] blk;
}

int main() {
    std::cout << "Starting heavy benchmark...\n";

    // 4 CPU threads
    std::thread t1(cpu_stress);
    std::thread t2(cpu_stress);
    std::thread t3(cpu_stress);
    std::thread t4(cpu_stress);
    std::thread t5(cpu_stress);
    std::thread t6(cpu_stress);
    std::thread t7(cpu_stress);
    std::thread t8(cpu_stress);

    // Память: выделяем до 2 ГБ по 64 МБ
    std::thread mem(memory_stress, 4096, 64);

    std::this_thread::sleep_for(std::chrono::seconds(30));
    running.store(false);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    mem.join();

    std::cout << "Benchmark finished\n";
    return 0;
}
