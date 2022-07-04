#pragma once

#include <iostream>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

template<typename T, size_t Items = 4096, size_t BlockSize = sizeof(T) * (1 + Items)>
class MemoryPool {
public:
    /* Member types */
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;
    typedef const T *const_pointer;
    typedef const T &const_reference;
    typedef size_t size_type;

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

    /* Member functions */
    MemoryPool() noexcept {
        currentBlock = nullptr;
        firstBlock = nullptr;
        currentSlot = nullptr;
        lastSlot = nullptr;
        freeSlots = nullptr;
    }

    MemoryPool(const MemoryPool &memoryPool) noexcept: MemoryPool() {}
    MemoryPool(MemoryPool &&memoryPool) noexcept {
        currentBlock = memoryPool.currentBlock;
        memoryPool.currentBlock = nullptr;
        currentSlot = memoryPool.currentSlot;
        firstBlock = memoryPool.firstBlock;
        lastSlot = memoryPool.lastSlot;
        freeSlots = memoryPool.freeSlots;
    }

    template<class U>
    MemoryPool(const MemoryPool<U> &memoryPool) noexcept: MemoryPool() {}

    ~MemoryPool() noexcept {
        slot_pointer curr = currentBlock;
        while (curr != nullptr) {
            slot_pointer prev = curr->pointer.prev;
            operator delete(reinterpret_cast<void *>(curr));
            curr = prev;
        }
    }

    unsigned int active = 0;
    unsigned int blocks = 0;

    MemoryPool &operator=(const MemoryPool &memoryPool) = delete;
    MemoryPool &operator=(MemoryPool &&memoryPool) noexcept {
        if (this != &memoryPool) {
            std::swap(currentBlock, memoryPool.currentBlock);
            currentSlot = memoryPool.currentSlot;
            lastSlot = memoryPool.lastSlot;
            firstBlock = memoryPool.firstBlock;
            freeSlots = memoryPool.freeSlots;
        }
        return *this;
    }

    pointer address(reference x) const noexcept {
        return &x;
    }

    const_pointer address(const_reference x) const noexcept {
        return &x;
    }

    // Can only allocate one object at a time. n and hint are ignored
    pointer allocate() {
        active++;
        if (freeSlots != nullptr) {
            pointer result = reinterpret_cast<pointer>(freeSlots);
            freeSlots = freeSlots->pointer.next;
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
            reinterpret_cast<slot_pointer>(p)->pointer = {.prev = nullptr, .next = freeSlots};
            freeSlots = reinterpret_cast<slot_pointer>(p);
        }
    }

    size_type max_size() const noexcept {
        size_type maxBlocks = -1 / BlockSize;
        return (BlockSize - sizeof(data_pointer)) / sizeof(slot_type) * maxBlocks;
    }

    template<class U, class... Args>
    void construct(U *p, Args &&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    template<class U>
    void destroy(U *p) {
        p->~U();
    }

    template<class... Args>
    pointer newElement(Args &&... args) {
        pointer result = allocate();
        construct<value_type>(result, std::forward<Args>(args)...);
        return result;
    }

    void deleteElement(pointer p) {
        if (p != nullptr) {
            p->~value_type();
            deallocate(p);
        }
    }

    void clear() {
        active = 0;
        freeSlots = nullptr;
        if (firstBlock) initializeBlock(firstBlock);
    }
private:
    slot_pointer currentBlock;
    slot_pointer firstBlock;

    slot_pointer currentSlot;
    slot_pointer lastSlot;
    slot_pointer freeSlots;

    size_type padPointer(data_pointer p, size_type align) const noexcept {
        uintptr_t result = reinterpret_cast<uintptr_t>(p);
        return ((align - result) % align);
    }

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
        // Pad block body to satisfy the alignment requirements for elements
        //data_pointer body = blockPoint + sizeof(slot_pointer);
        //size_type bodyPadding = padPointer(body, alignof(slot_type));
        return reinterpret_cast<slot_pointer>(blockPoint + sizeof(slot_type));
    }

    void initializeBlock(slot_pointer nextBlock) {
        currentBlock = nextBlock;
        //// Pad block body to satisfy the alignment requirements for elements
        //data_pointer_ body = newBlock + sizeof(slot_pointer_);
        //size_type bodyPadding = padPointer(body, alignof(slot_type_));
        currentSlot = blockStartSlot(nextBlock); // reinterpret_cast<slot_pointer_>(body + bodyPadding);
        lastSlot = reinterpret_cast<slot_pointer>(reinterpret_cast<data_pointer>(nextBlock) + BlockSize - sizeof(slot_type) + 1);
    }

    static_assert(BlockSize>=2 * sizeof(slot_type), "BlockSize too small.");
};