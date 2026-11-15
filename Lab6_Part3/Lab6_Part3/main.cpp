#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <chrono>
#include <array>
#include <vector>

// 1. Два споживачі (базовий)
const int CONSUMERS_COUNT = 2;
const int DELAY_FACTOR = 150;
const bool USE_NOTIFY_ONE = true;

// 2. Один споживач
// const int CONSUMERS_COUNT = 1;
// const int DELAY_FACTOR = 150;
// const bool USE_NOTIFY_ONE = true;

// 3. Чотири споживачі
// const int CONSUMERS_COUNT = 4;
// const int DELAY_FACTOR = 150;
// const bool USE_NOTIFY_ONE = true;

// 4. Більша затримка
// const int CONSUMERS_COUNT = 2;
// const int DELAY_FACTOR = 500;
// const bool USE_NOTIFY_ONE = true;

// 5. Без notify_one
// const int CONSUMERS_COUNT = 2;
// const int DELAY_FACTOR = 150;
// const bool USE_NOTIFY_ONE = false;

using Clock = std::chrono::steady_clock;

struct Part
{
    int id;
    int type;
    std::string name;
    int complexity;
};

const int PART_COUNT = 12;
const std::array<std::string, 3> PART_NAMES = { "Body", "Engine", "Wheel" };

std::queue<Part> parts;
std::mutex queueMutex;
std::condition_variable condition;
bool finished = false;

std::mutex outputMutex;

std::mutex assemblyMutex;
std::array<int, 3> stock{ 0, 0, 0 };
int finishedProducts = 0;

void producer()
{
    for (int i = 1; i <= PART_COUNT; ++i)
    {
        int type = (i - 1) % 3;
        Part part{ i, type, PART_NAMES[type], i % 3 + 1 };
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            parts.push(part);
        }
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Producer] Created part " << part.id
                << ", type: " << part.name << '\n';
        }
        if (USE_NOTIFY_ONE)
            condition.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        finished = true;
    }
    condition.notify_all();
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << "[Producer] Generation finished\n";
}

void consumer(int consumerId)
{
    while (true)
    {
        Part part;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [] { return !parts.empty() || finished; });
            if (parts.empty() && finished)
                break;
            part = parts.front();
            parts.pop();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(part.complexity * DELAY_FACTOR));

        bool assembled = false;
        int total = 0;
        {
            std::lock_guard<std::mutex> lock(assemblyMutex);
            ++stock[part.type];
            if (stock[0] > 0 && stock[1] > 0 && stock[2] > 0)
            {
                --stock[0];
                --stock[1];
                --stock[2];
                ++finishedProducts;
                assembled = true;
                total = finishedProducts;
            }
        }
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "[Consumer " << consumerId << "] Processed part "
                << part.id << ", type: " << part.name
                << ", complexity: " << part.complexity;
            if (assembled)
                std::cout << "  -> PRODUCT ASSEMBLED (total: " << total << ")";
            std::cout << '\n';
        }
    }
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << "[Consumer " << consumerId << "] Finished\n";
}

int main()
{
    std::cout << "Consumers: " << CONSUMERS_COUNT
        << ", delay factor: " << DELAY_FACTOR
        << ", notify_one: " << (USE_NOTIFY_ONE ? "on" : "off") << "\n\n";

    auto startTime = Clock::now();

    std::thread producerThread(producer);
    std::vector<std::thread> consumers;
    for (int i = 1; i <= CONSUMERS_COUNT; ++i)
        consumers.emplace_back(consumer, i);

    producerThread.join();
    for (auto& c : consumers)
        c.join();

    auto endTime = Clock::now();

    std::cout << "\nAll parts have been processed.\n"
        << "Finished products: " << finishedProducts << '\n'
        << "Leftover parts (Body/Engine/Wheel): "
        << stock[0] << '/' << stock[1] << '/' << stock[2] << '\n'
        << "Total time: "
        << std::chrono::duration<double, std::milli>(endTime - startTime).count()
        << " ms\n";
    return 0;
}