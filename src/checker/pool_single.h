#pragma once

#include <iostream>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <span>
#include "../core.h"

/**
 * Block is memory (Items + 1) * sizeof<T>. Plus 1 because first entry is block header.
 * Block header points to next and previous block.
 */
template<class T, size_t Items = 4096, size_t GCQueueSize = Items / 2, size_t BlockSize = sizeof(T) * (1 + Items)>
class PoolSingle {
public:
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;

    std::array<pointer, GCQueueSize> gcQueue;
    unsigned int gcQueued = 0;

    union Slot {
        value_type element;
        struct Pointer {
            Slot *prev;
            Slot *next;
        } pointer;
    };

    typedef char *data_pointer;
    typedef Slot slot_type;
    typedef Slot *slot_pointer;
    static_assert(BlockSize>=2 * sizeof(slot_type), "BlockSize too small.");

    ~PoolSingle() noexcept {
        slot_pointer curr = currentBlock;
        while (curr != nullptr) {
            slot_pointer prev = curr->pointer.prev;
            operator delete(reinterpret_cast<void *>(curr));
            curr = prev;
        }
    }

    unsigned int active = 0;
    unsigned int blocks = 0;

    pointer allocate() {
        active++;
        if (freeSlot != nullptr) {
            pointer result = reinterpret_cast<pointer>(freeSlot);
            freeSlot = freeSlot->pointer.next;
            return result;
        } else {
            if (currentSlot>=lastSlot) {
                allocateBlock();
            }
            return reinterpret_cast<pointer>(currentSlot++);
        }
    }

    void deallocate(pointer p) {
        if (p != nullptr) {
            active--;
            auto slot = reinterpret_cast<slot_pointer>(p);
            slot->pointer = {.prev = nullptr, .next = freeSlot};
            freeSlot = slot;
        }
    }

    template<class... Args>
    pointer construct(Args &&... args) {
        pointer result = allocate();
        new(result) T(std::forward<Args>(args)...);
        return result;
    }

    void destruct(T *p) {
        p->~T();
        deallocate(p);
    }

    void clear() {
        active = 0;
        freeSlot = nullptr;
        gcQueued = 0;
        if (firstBlock) initializeBlock(firstBlock);
    }

    void gc(pointer p) {
        //flush queued items
        if (gcQueued>=GCQueueSize) gcFlush();
        gcQueue[gcQueued++] = p;
    }

    void gcFlush() {
        for (unsigned int i = 0; i<gcQueued; i++) destruct(gcQueue[i]);
        gcQueued = 0;
    }

private:
    slot_pointer currentBlock = nullptr;
    slot_pointer firstBlock = nullptr;

    slot_pointer currentSlot = nullptr;
    slot_pointer lastSlot = nullptr;
    slot_pointer freeSlot = nullptr;

    void allocateBlock() {
        if (currentBlock && reinterpret_cast<slot_pointer>(currentBlock)->pointer.next) {
            initializeBlock(reinterpret_cast<slot_pointer>(currentBlock)->pointer.next);
        } else {
            blocks++;
            // Allocate space for the new block and store a pointer to the previous one
            data_pointer newBlock = reinterpret_cast<data_pointer>(operator new(BlockSize));
            reinterpret_cast<slot_pointer>(newBlock)->pointer = {.prev = currentBlock, .next = nullptr};
            setNextBlock(reinterpret_cast<slot_pointer>(newBlock));
        }
    }

    void setNextBlock(slot_pointer nextBlock) {
        if (currentBlock) currentBlock->pointer.next = nextBlock;
        if (!firstBlock) firstBlock = nextBlock;
        initializeBlock(nextBlock);
    }

    slot_pointer blockStartSlot(slot_pointer block) {
        auto blockPoint = reinterpret_cast<data_pointer>(block);
        return reinterpret_cast<slot_pointer>(blockPoint + sizeof(slot_type));
    }

    void initializeBlock(slot_pointer nextBlock) {
        currentBlock = nextBlock;
        currentSlot = blockStartSlot(nextBlock);
        lastSlot = reinterpret_cast<slot_pointer>(reinterpret_cast<data_pointer>(nextBlock) + BlockSize - sizeof(slot_type) + 1);
    }
};