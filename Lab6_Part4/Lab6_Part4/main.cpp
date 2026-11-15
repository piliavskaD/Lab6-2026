#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <thread>

using Clock = std::chrono::steady_clock;

void addSequential(const std::vector<int>& a, const std::vector<int>& b, std::vector<int>& c)
{
    for (size_t i = 0; i < a.size(); ++i)
        c[i] = a[i] + b[i];
}

void addRange(const std::vector<int>& a, const std::vector<int>& b,
    std::vector<int>& c, size_t begin, size_t end)
{
    for (size_t i = begin; i < end; ++i)
        c[i] = a[i] + b[i];
}

void addParallel(const std::vector<int>& a, const std::vector<int>& b,
    std::vector<int>& c, int k)
{
    std::vector<std::future<void>> futures;
    size_t n = a.size();
    size_t chunk = n / k;
    for (int t = 0; t < k; ++t)
    {
        size_t begin = t * chunk;
        size_t end = (t == k - 1) ? n : begin + chunk;
        futures.push_back(std::async(std::launch::async,
            addRange, std::cref(a), std::cref(b),
            std::ref(c), begin, end));
    }
    for (auto& f : futures)
        f.get();
}

template <class F>
double measure(F f, int repeats = 5)
{
    std::vector<double> times;
    for (int r = 0; r < repeats; ++r)
    {
        auto s = Clock::now();
        f();
        auto e = Clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(e - s).count());
    }
    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}

int main()
{
    const std::vector<size_t> sizes = { 1'000'000, 10'000'000, 30'000'000 };
    const std::vector<int> taskCounts = { 2, 4, 8 };
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << "\n\n";

    std::cout << std::left << std::setw(12) << "Size" << std::setw(8) << "Tasks"
        << std::setw(14) << "Seq, ms" << std::setw(14) << "Par, ms"
        << std::setw(12) << "Speedup" << std::setw(12) << "Efficiency"
        << "Correct\n";

    for (size_t n : sizes)
    {
        std::vector<int> a(n), b(n), cSeq(n), cPar(n);
        std::iota(a.begin(), a.end(), 0);
        for (size_t i = 0; i < n; ++i)
            b[i] = static_cast<int>(i % 1000);

        double tSeq = measure([&] { addSequential(a, b, cSeq); });

        for (int k : taskCounts)
        {
            double tPar = measure([&] { addParallel(a, b, cPar, k); });
            bool ok = (cSeq == cPar);
            double S = tSeq / tPar;
            double E = S / k;
            std::cout << std::left << std::setw(12) << n << std::setw(8) << k
                << std::fixed << std::setprecision(3)
                << std::setw(14) << tSeq << std::setw(14) << tPar
                << std::setw(12) << S << std::setw(12) << E
                << (ok ? "yes" : "NO") << '\n';
        }
    }
    return 0;
}