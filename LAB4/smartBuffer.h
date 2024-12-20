#ifndef SMARTBUFFER_H
#define SMARTBUFFER_H

#include "monitor.h"
#include <queue>
#include <iostream>

template <typename T>
class SmartBuffer : public Monitor {
private:
    std::queue<T> buffer;
    int maxSize;
    int id;
    Condition isEmpty;
    Condition isFull;

public:
    SmartBuffer(int size, int bufferId) : maxSize(size), id(bufferId) {}

    void push(const T& item) {
        enter();
        if (buffer.size() >= maxSize) {
            wait(isFull);
        }
        buffer.push(item);
        std::cout << "Buffer " << id << ": Produced -> " << item << std::endl;
        signal(isEmpty);
        leave();
    }

    T pop() {
        enter();
        if (buffer.empty()) {
            wait(isEmpty);
        }
        T item = buffer.front();
        buffer.pop();
        std::cout << "Buffer " << id << ": Consumed -> " << item << std::endl;
        signal(isFull);
        leave();
        return item;
    }
};

#endif
