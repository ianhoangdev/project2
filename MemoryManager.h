#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <functional>
#include <cstddef>

class MemoryManager {
public:

    MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator);
    ~MemoryManager();

    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();

private:

    struct MemoryBlock {
        bool is_free;
        size_t size;
        MemoryBlock *prev;
        MemoryBlock *next;
    };

    unsigned wordSize;
    std::function<int(int, void *)> allocator;
    void *memoryStart;
    void *alignedStart;
    size_t memoryLimit;
    MemoryBlock *headNode;

    void mergeNext(MemoryBlock *block);
    void splitBlock(MemoryBlock *block, size_t requiredSize);
    size_t align(size_t size);
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

#endif