#include "smartBuffer.h"
#include <thread>
#include <vector>
#include <cstdlib>
#include <chrono>

const int BUFFER_SIZE = 2;
const int NUM_BUFFERS = 4;
const int NUM_CONSUMERS = 4;

void producer(SmartBuffer<int>& buffer, int producer_id) {
    while (true) {
        int item = std::rand() % 100;
        buffer.push(item);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void superProducer(std::vector<SmartBuffer<int>>& buffers) {
    while (true) {
        for (auto& buffer : buffers) {
            int item = std::rand() % 100;
            buffer.push(item);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
}

void consumer(SmartBuffer<int>& buffer, int consumer_id) {
    while (true) {
        int item = buffer.pop();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    std::vector<SmartBuffer<int>> buffers;
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        buffers.emplace_back(BUFFER_SIZE, i + 1);
    }

    std::vector<std::thread> producers;
    producers.emplace_back(producer, std::ref(buffers[0]), 1);
    producers.emplace_back(producer, std::ref(buffers[NUM_BUFFERS - 1]), 2);
    producers.emplace_back(superProducer, std::ref(buffers));

    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer, std::ref(buffers[i]), i + 1);
    }

    for (auto& prod : producers) {
        prod.join();
    }

    for (auto& cons : consumers) {
        cons.join();
    }

    return 0;
}
