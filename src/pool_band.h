#pragma once

#include <iostream>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <span>

class PoolBand {
public:
    union Slot {
        struct Pointer {
            Slot *prev;
            Slot *next;
        } pointer;
    };

    typedef Slot *slot_pointer;

    size_t blockSize;

    explicit PoolBand(size_t blockSize = 1024 * 100): blockSize(blockSize) {}

    ~PoolBand() noexcept {
        slot_pointer curr = currentBlock;
        while (curr != nullptr) {
            slot_pointer prev = curr->pointer.prev;
            operator delete(reinterpret_cast<char *>(curr));
            curr = prev;
        }
    }

    unsigned int active = 0;
    unsigned int blocks = 0;

    template<class T>
    T *allocate() {
        active++;
        auto size = sizeof(T);
        //std::cout<<"allocate "<<typeid(T).name()<<" "<<size<<" of blocksize " << blockSize << " of " <<end - currentSlot<<"\n";
        if (currentSlot + size>end) {
            allocateBlock();
        }
        auto res = reinterpret_cast<T *>(currentSlot);
        currentSlot += size;
        return res;
    }

    template<class T, class... Args>
    T *construct(Args &&... args) {
        auto result = allocate<T>();
        new(result) T(std::forward<Args>(args)...);
        return result;
    }

    template<class T>
    void destruct(T *p) {
        p->~T();
        //deallocate(p);
    }

    void clear() {
        active = 0;
        if (firstBlock) initializeBlock(firstBlock);
    }
private:
    slot_pointer currentBlock = nullptr;
    slot_pointer firstBlock = nullptr;

    char *currentSlot = nullptr;
    char *end = nullptr;

    void allocateBlock() {
        if (currentBlock && reinterpret_cast<slot_pointer>(currentBlock)->pointer.next) {
            initializeBlock(reinterpret_cast<slot_pointer>(currentBlock)->pointer.next);
        } else {
            blocks++;
            // Allocate space for the new block and store a pointer to the previous one
            auto newBlock = reinterpret_cast<char *>(operator new(sizeof(Slot) + blockSize));
            reinterpret_cast<slot_pointer>(newBlock)->pointer = {.prev = currentBlock, .next = nullptr};
            setNextBlock(reinterpret_cast<slot_pointer>(newBlock));
        }
    }

    void setNextBlock(slot_pointer nextBlock) {
        if (currentBlock) currentBlock->pointer.next = nextBlock;
        if (!firstBlock) firstBlock = nextBlock;
        initializeBlock(nextBlock);
    }

    void initializeBlock(slot_pointer nextBlock) {
        currentBlock = nextBlock;
        currentSlot = reinterpret_cast<char *>(nextBlock) + sizeof(Slot);
        end = currentSlot + blockSize;
    }
};