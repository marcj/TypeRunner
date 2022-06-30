#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

template<typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    /* Member types */
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;
    typedef const T *const_pointer;
    typedef const T &const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_swap;

    template<typename U>
    struct rebind {
        typedef MemoryPool<U> other;
    };

    /* Member functions */
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool &memoryPool) noexcept;
    MemoryPool(MemoryPool &&memoryPool) noexcept;
    template<class U>
    MemoryPool(const MemoryPool<U> &memoryPool) noexcept;

    ~MemoryPool() noexcept;

    unsigned int active = 0;

    MemoryPool &operator=(const MemoryPool &memoryPool) = delete;
    MemoryPool &operator=(MemoryPool &&memoryPool) noexcept;

    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;

    // Can only allocate one object at a time. n and hint are ignored
    pointer allocate(size_type n = 1, const_pointer hint = 0);
    void deallocate(pointer p, size_type n = 1);

    // Schedules for garbage collection
    void gc(pointer p);

    size_type max_size() const noexcept;

    template<class U, class... Args>
    void construct(U *p, Args &&... args);

    template<class U>
    void destroy(U *p);

    template<class... Args>
    pointer newElement(Args &&... args);
    void deleteElement(pointer p);

private:
    union Slot_ {
        value_type element;
        Slot_ *next;
    };

    typedef char *data_pointer_;
    typedef Slot_ slot_type_;
    typedef Slot_ *slot_pointer_;

    slot_pointer_ currentBlock_;
    slot_pointer_ currentSlot_;
    slot_pointer_ lastSlot_;
    slot_pointer_ freeSlots_;

    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
    void allocateBlock();

    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");
};

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const noexcept {
    uintptr_t result = reinterpret_cast<uintptr_t>(p);
    return ((align - result) % align);
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
noexcept {
    currentBlock_ = nullptr;
    currentSlot_ = nullptr;
    lastSlot_ = nullptr;
    freeSlots_ = nullptr;
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool &memoryPool)
noexcept :
        MemoryPool() {}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool &&memoryPool)
noexcept {
    currentBlock_ = memoryPool.currentBlock_;
    memoryPool.currentBlock_ = nullptr;
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots;
}

template<typename T, size_t BlockSize>
template<class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U> &memoryPool)
noexcept :
        MemoryPool() {}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize> &
MemoryPool<T, BlockSize>::operator=(MemoryPool &&memoryPool)
noexcept {
    if (this != &memoryPool) {
        std::swap(currentBlock_, memoryPool.currentBlock_);
        currentSlot_ = memoryPool.currentSlot_;
        lastSlot_ = memoryPool.lastSlot_;
        freeSlots_ = memoryPool.freeSlots_;
    }
    return *this;
}

template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
noexcept {
    slot_pointer_ curr = currentBlock_;
    while (curr != nullptr) {
        slot_pointer_ prev = curr->next;
        operator delete(reinterpret_cast<void *>(curr));
        curr = prev;
    }
}

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x)
const noexcept {
    return &x;
}

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const noexcept {
    return &x;
}

template<typename T, size_t BlockSize>
void
MemoryPool<T, BlockSize>::allocateBlock() {
    // Allocate space for the new block and store a pointer to the previous one
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>
    (operator new(BlockSize));
    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newBlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<slot_pointer_>
    (newBlock + BlockSize - sizeof(slot_type_) + 1);
}

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint) {
    active++;
    if (freeSlots_ != nullptr) {
        pointer result = reinterpret_cast<pointer>(freeSlots_);
        freeSlots_ = freeSlots_->next;
        return result;
    } else {
        if (currentSlot_ >= lastSlot_) {
            allocateBlock();
        }
        return reinterpret_cast<pointer>(currentSlot_++);
    }
}

template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::gc(pointer p) {
    deallocate(p);
//    if (p != nullptr) {
//        reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
//        freeSlots_ = reinterpret_cast<slot_pointer_>(p);
//    }
}

template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n) {
    active--;
    if (p != nullptr) {
        reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
        freeSlots_ = reinterpret_cast<slot_pointer_>(p);
    }
}

template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept {
    size_type maxBlocks = -1 / BlockSize;
    return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}

template<typename T, size_t BlockSize>
template<class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U *p, Args &&... args) {
    new(p) U(std::forward<Args>(args)...);
}

template<typename T, size_t BlockSize>
template<class U>
inline void
MemoryPool<T, BlockSize>::destroy(U *p) {
    p->~U();
}

template<typename T, size_t BlockSize>
template<class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args &&... args) {
    pointer result = allocate();
    construct<value_type>(result, std::forward<Args>(args)...);
    return result;
}

template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p) {
    if (p != nullptr) {
        p->~value_type();
        deallocate(p);
    }
}