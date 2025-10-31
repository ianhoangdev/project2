#pragma once

#include <functional>
#include <list>
#include <cstddef>
#include <cstdint>

struct MemoryBlock {
    size_t offset;
    size_t length;
    bool is_hole;

    MemoryBlock(size_t o, size_t l, bool h) : offset(o), length(l), is_hole(h) {}
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

class MemoryManager {
public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    ~MemoryManager();

    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);

    void *getList();
    void *getBitmap();
    unsigned getWordSize() { return m_wordSize; }
    void *getMemoryStart() { return m_memoryStart; }
    unsigned getMemoryLimit() { return m_memoryLimit; }

private:
    void mergeHoles();

    unsigned m_wordSize;
    size_t m_totalWords;
    unsigned m_memoryLimit;
    void *m_memoryStart;
    std::function<int(int, void *)> m_allocator; 
    std::list<MemoryBlock> m_blocks; 
};