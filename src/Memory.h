#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

class LinearAllocator
{
public:
    explicit LinearAllocator(std::size_t totalBytes)
        : buffer(totalBytes > 0 ? new std::uint8_t[totalBytes] : nullptr),
          offset(0),
          used(0),
          capacity(totalBytes)
    {
    }

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    LinearAllocator(LinearAllocator&& other) noexcept
        : buffer(other.buffer),
          offset(other.offset),
          used(other.used),
          capacity(other.capacity)
    {
        other.buffer = nullptr;
        other.offset = 0;
        other.used = 0;
        other.capacity = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& other) noexcept
    {
        if (this != &other)
        {
            delete[] buffer;
            buffer = other.buffer;
            offset = other.offset;
            used = other.used;
            capacity = other.capacity;

            other.buffer = nullptr;
            other.offset = 0;
            other.used = 0;
            other.capacity = 0;
        }

        return *this;
    }

    void* alloc(std::size_t bytes)
    {
        if (buffer == nullptr || bytes == 0)
        {
            return nullptr;
        }

        constexpr std::size_t alignment = 8;
        const std::size_t alignedOffset = (offset + (alignment - 1)) & ~(alignment - 1);

        if (alignedOffset > capacity || bytes > (capacity - alignedOffset))
        {
            return nullptr;
        }

        void* result = buffer + alignedOffset;
        offset = alignedOffset + bytes;
        used = offset;
        return result;
    }

    void reset() noexcept
    {
        offset = 0;
        used = 0;
    }

    ~LinearAllocator()
    {
        delete[] buffer;
    }

    std::size_t used;
    std::size_t capacity;

private:
    std::uint8_t* buffer;
    std::size_t offset;
};

template<typename T>
class PoolAllocator
{
public:
    explicit PoolAllocator(std::size_t count)
        : storage(nullptr),
          freeList(nullptr),
          capacity(count)
    {
        static_assert(sizeof(T) >= sizeof(void*), "PoolAllocator requires T to hold a freelist pointer.");
        static_assert(alignof(T) >= alignof(void*), "PoolAllocator requires T alignment compatible with a pointer.");

        if (capacity == 0)
        {
            return;
        }

        storage = ::operator new(sizeof(T) * capacity, std::align_val_t(alignof(T)));
        auto* bytes = static_cast<std::uint8_t*>(storage);

        for (std::size_t index = 0; index < capacity; ++index)
        {
            void* slot = bytes + (index * sizeof(T));
            void* next = (index + 1 < capacity) ? (bytes + ((index + 1) * sizeof(T))) : nullptr;
            *static_cast<void**>(slot) = next;
        }

        freeList = storage;
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    PoolAllocator(PoolAllocator&& other) noexcept
        : storage(other.storage),
          freeList(other.freeList),
          capacity(other.capacity)
    {
        other.storage = nullptr;
        other.freeList = nullptr;
        other.capacity = 0;
    }

    PoolAllocator& operator=(PoolAllocator&& other) noexcept
    {
        if (this != &other)
        {
            release();

            storage = other.storage;
            freeList = other.freeList;
            capacity = other.capacity;

            other.storage = nullptr;
            other.freeList = nullptr;
            other.capacity = 0;
        }

        return *this;
    }

    T* alloc() noexcept
    {
        if (freeList == nullptr)
        {
            return nullptr;
        }

        void* slot = freeList;
        freeList = *static_cast<void**>(freeList);
        return static_cast<T*>(slot);
    }

    void free(T* ptr) noexcept
    {
        if (ptr == nullptr)
        {
            return;
        }

        *reinterpret_cast<void**>(ptr) = freeList;
        freeList = ptr;
    }

    ~PoolAllocator()
    {
        release();
    }

private:
    void release() noexcept
    {
        if (storage != nullptr)
        {
            ::operator delete(storage, std::align_val_t(alignof(T)));
            storage = nullptr;
            freeList = nullptr;
            capacity = 0;
        }
    }

    void* storage;
    void* freeList;
    std::size_t capacity;
};
