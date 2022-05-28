#pragma once

#include <stdint.h>
#include <stdlib.h>

struct PersistentLinearAllocator {
    void* Block; // pointer to block header
    size_t Offset; // offset within the block
    size_t CurrentBlockSize; // size of the current block
    size_t BlockSize; // general block size
};

void Init(PersistentLinearAllocator& allocator, size_t blockSize = 4096);
void* Allocate(PersistentLinearAllocator& allocator, size_t size, size_t align);
void* Allocate(PersistentLinearAllocator& allocator, size_t size);
template<typename T>
T* Allocate(PersistentLinearAllocator& allocator) {
    return (T*)Allocate(allocator, sizeof(T));
}

template<typename T>
T* AllocateArray(PersistentLinearAllocator& allocator, size_t size) {
    return (T*)Allocate(allocator, sizeof(T) *size);
}
void Wipe(PersistentLinearAllocator& allocator);