#pragma once

#include <iostream>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>
#include "math.h"
#include <array>
#include <type_traits>

/**
 * Block is memory (Items + 1) * sizeof<T>. Plus 1 because first entry is block header.
 * Block header points to next and previous block.
 */
template<typename T, size_t Items = 4096, size_t GCQueueSize = Items / 2, size_t BlockSize = sizeof(T) * (1 + Items)>
class PoolArray {
public:
    unsigned int active = 0;

    union Slot {
        T element;
        struct Pointer {
            Slot *prev;
            Slot *next;
        } header;
    };

    typedef char *data_pointer;
    typedef Slot slot_type;
    typedef Slot *slot_pointer;

    static_assert(BlockSize>=2 * sizeof(slot_type), "BlockSize too small.");
    static_assert(Items>=GCQueueSize, "Items need to be bigger than GCQueueSize");

    struct Pool {
        Slot *currentBlock = nullptr;
        Slot *firstBlock = nullptr;

        Slot *currentSlot = nullptr;
        Slot *lastSlot = nullptr;
        Slot *freeSlot = nullptr;
        unsigned int blocks = 0;
        unsigned int slotSize;

        std::vector<T *> gcQueue;
        unsigned int gcQueued = 0;
        unsigned int gcQueueSize = 0;

        explicit Pool(unsigned int slotSize): slotSize(slotSize), gcQueueSize(ceil(GCQueueSize/slotSize)) {
            gcQueue.resize(gcQueueSize);
        }

        void deallocate(const std::span<T> &span) {
            T *p = &span[0];
            auto slot = reinterpret_cast<slot_pointer>(p);
            slot->header = {.prev = nullptr, .next = freeSlot};
            freeSlot = slot;
        }

        void destruct(const std::span<T> &span) {
            for (auto &&item: span) item.~T();
            deallocate(span);
        }

        void gc(const std::span<T> &span) {
            //flush queued items
            if (gcQueued>=gcQueueSize) gcFlush();
            gcQueue[gcQueued++] = &span[0];
        }

        void gcFlush() {
            for (unsigned int i = 0; i<gcQueueSize; i++) {
                //we don't need to call destructor since TypeRef doesn't have one
                //for (unsigned int j = 0; j<slotSize; j++) {
                //    (gcQueue[i][j]).~T();
                //}
                auto slot = reinterpret_cast<slot_pointer>(gcQueue[i]);
                slot->header = {.prev = nullptr, .next = freeSlot};
                freeSlot = slot;
            }
            gcQueued = 0;
        }
    };

    constexpr static unsigned int poolAmount = 11;
    std::array<Pool, poolAmount> pools = {Pool(1), Pool(2), Pool(4), Pool(8), Pool(16), Pool(32), Pool(64), Pool(128), Pool(256), Pool(512), Pool(1024)};

    PoolArray() noexcept {}

    ~PoolArray() noexcept {
        for (auto &&pool: pools) {
            slot_pointer curr = pool.currentBlock;
            while (curr != nullptr) {
                slot_pointer prev = curr->header.prev;
                operator delete(reinterpret_cast<void *>(curr));
                curr = prev;
            }
        }
    }

    unsigned int poolIndex(unsigned int size) {
        if (size>1024) size = 1024;
        return ceil(log2(size));
    }

    Pool &getPool(unsigned int size) {
        return pools[poolIndex(size)];
    }

    std::span<T> allocate(unsigned int size) {
        auto &pool = getPool(size);
        active += size;
        if (pool.freeSlot != nullptr) {
            T *result = reinterpret_cast<T *>(pool.freeSlot);
            pool.freeSlot = pool.freeSlot->header.next;
            return {result, size};
        } else {
            if (pool.currentSlot + pool.slotSize - 1>=pool.lastSlot) {
                allocateBlock(pool);
            }
            auto result = reinterpret_cast<T *>(pool.currentSlot + 1);
            pool.currentSlot += pool.slotSize;
            return {result, size};
        }
    }

    void deallocate(const std::span<T> &span) {
        active -= span.size();
        auto &pool = getPool(span.size());
        pool.deallocate(span);
    }

    std::span<T> construct(unsigned int size) {
        auto span = allocate(size);
        for (auto &&item: span) new(&item) T();
        return span;
    }

    void destruct(const std::span<T> &span) {
        for (auto &&item: span) item.~T();
        deallocate(span);
    }

    void clear() {
        active = 0;
        for (auto &&pool: pools) {
            pool.freeSlot = nullptr;
            pool.gcQueued = 0;
            if (pool.firstBlock) initializeBlock(pool, pool.firstBlock);
        }
    }

    void gc(const std::span<T> &span) {
        auto &pool = getPool(span.size());
        pool.gc(span);
    }

private:

    void allocateBlock(Pool &pool) {
        if (pool.currentBlock && reinterpret_cast<slot_pointer>(pool.currentBlock)->header.next) {
            initializeBlock(pool, reinterpret_cast<slot_pointer>(pool.currentBlock)->header.next);
        } else {
            pool.blocks++;
            // Allocate space for the new block and store a pointer to the previous one
            data_pointer newBlock = reinterpret_cast<data_pointer>(operator new(BlockSize));
            reinterpret_cast<slot_pointer>(newBlock)->header = {.prev = pool.currentBlock, .next = nullptr};
            setNextBlock(pool, reinterpret_cast<slot_pointer>(newBlock));
        }
    }

    void setNextBlock(Pool &pool, slot_pointer nextBlock) {
        if (pool.currentBlock) pool.currentBlock->header.next = nextBlock;
        if (!pool.firstBlock) pool.firstBlock = nextBlock;
        initializeBlock(pool, nextBlock);
    }

    slot_pointer blockStartSlot(slot_pointer block) {
        auto blockPoint = reinterpret_cast<data_pointer>(block);
        return reinterpret_cast<slot_pointer>(blockPoint + sizeof(slot_type));
    }

    void initializeBlock(Pool &pool, slot_pointer nextBlock) {
        pool.currentBlock = nextBlock;
        pool.currentSlot = blockStartSlot(nextBlock);
        pool.lastSlot = reinterpret_cast<slot_pointer>(reinterpret_cast<data_pointer>(nextBlock) + BlockSize - sizeof(slot_type) + 1);
    }
};