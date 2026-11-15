#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iomanip>

const int N = 17;
const int ITERATIONS = N * 100000;

volatile int counterRaw = 0;
void incrementRaw()
{
    for (int i = 0; i < ITERATIONS; ++i)
        counterRaw = counterRaw + 1;
}

int counterMutexVal = 0;
std::mutex counterMutex;
void incrementMutex()
{
    for (int i = 0; i < ITERATIONS; ++i)
    {
        std::lock_guard<std::mutex> lock(counterMutex);
        ++counterMutexVal;
    }
}

std::atomic<int> counterAtomic{ 0 };
void incrementAtomic()
{
    for (int i = 0; i < ITERATIONS; ++i)
        ++counterAtomic;
}

template <class F>
double runTwoThreads(F f)
{
    auto start = std::chrono::steady_clock::now();
    std::thread t1(f);
    std::thread t2(f);
    t1.join();
    t2.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main()
{
    const int expected = 2 * ITERATIONS;
    const int RUNS = 10;
    std::cout << "Expected value: " << expected << "\n\n";

    int wrong = 0;
    std::cout << "[No synchronization]\n";
    for (int r = 1; r <= RUNS; ++r)
    {
        counterRaw = 0;
        runTwoThreads(incrementRaw);
        int actual = counterRaw;
        if (actual != expected) ++wrong;
        std::cout << " Run " << std::setw(2) << r << ": actual = " << actual
            << (actual == expected ? "  OK\n" : "  MISMATCH\n");
    }
    std::cout << " Wrong runs: " << wrong << " of " << RUNS << "\n\n";

    std::cout << "[std::mutex]\n";
    double tMutex = 0;
    for (int r = 1; r <= 5; ++r)
    {
        counterMutexVal = 0;
        double t = runTwoThreads(incrementMutex);
        tMutex += t;
        std::cout << " Run " << r << ": actual = " << counterMutexVal
            << (counterMutexVal == expected ? "  OK" : "  MISMATCH")
            << ", " << std::fixed << std::setprecision(2) << t << " ms\n";
    }

    std::cout << "\n[std::atomic]\n";
    double tAtomic = 0;
    for (int r = 1; r <= 5; ++r)
    {
        counterAtomic = 0;
        double t = runTwoThreads(incrementAtomic);
        tAtomic += t;
        std::cout << " Run " << r << ": actual = " << counterAtomic.load()
            << (counterAtomic.load() == expected ? "  OK" : "  MISMATCH")
            << ", " << t << " ms\n";
    }

    std::cout << "\nAverage time: mutex = " << tMutex / 5
        << " ms, atomic = " << tAtomic / 5 << " ms\n";
    return 0;
}