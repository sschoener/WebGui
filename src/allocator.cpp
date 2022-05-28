#include "allocator.h"

#include <stdlib.h>
#include <assert.h>

void Init(PersistentLinearAllocator& allocator, size_t blockSize)
{
    allocator.Block = nullptr;
    allocator.Offset = 0;
    allocator.CurrentBlockSize = 0;
    allocator.BlockSize = blockSize;
}

struct BlockHeader {
    BlockHeader* PreviousBlock;
    BlockHeader* NextBlock;
    size_t BlockSize;
};

static uint8_t* Block(BlockHeader* header)
{
    return ((uint8_t*)header) - header->BlockSize;
}

void* Allocate(PersistentLinearAllocator& allocator, size_t size)
{
    return Allocate(allocator, size, 0);
}

void* Allocate(PersistentLinearAllocator& allocator, size_t size, size_t align)
{
    BlockHeader* header = (BlockHeader*) allocator.Block;
    if (align != 0 && header) {
        size_t off = allocator.Offset % align;
        if (off != 0)
            allocator.Offset += align - off;
    }
    
    if (allocator.CurrentBlockSize - allocator.Offset < size) {
        // allocate a new block
        size_t allocSize = allocator.BlockSize > size ? allocator.BlockSize : size;
        uint8_t* block = (uint8_t*)malloc(allocSize + sizeof(BlockHeader));
        BlockHeader* newHeader = (BlockHeader*)(block + allocSize);
        newHeader->PreviousBlock = header;
        newHeader->NextBlock = nullptr;
        newHeader->BlockSize = allocSize;
        allocator.CurrentBlockSize = allocSize;
        allocator.Offset = 0;
        if (header)
            header->NextBlock = newHeader;
        allocator.Block = newHeader;
    }
    uint8_t* ptr = Block((BlockHeader*)allocator.Block) + allocator.Offset;
    allocator.Offset += size;
    assert(allocator.Offset <= allocator.CurrentBlockSize);
    return (void*)ptr;
}

void Wipe(PersistentLinearAllocator& allocator)
{
    BlockHeader* header = (BlockHeader*)allocator.Block;
    while (header) {
        BlockHeader* tmp = header->PreviousBlock;
        free(Block(header));
        header = tmp;
    }
} 