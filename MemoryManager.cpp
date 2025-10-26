#include "MemoryManager.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
    : m_wordSize(wordSize), m_allocator(allocator), m_totalWords(0),
      m_memoryLimit(0), m_memoryStart(nullptr) {
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    if (m_memoryStart) {
        shutdown();
    }

    if (sizeInWords > 65535) {
        sizeInWords = 65535;
    }

    m_totalWords = sizeInWords;
    m_memoryLimit = m_totalWords * m_wordSize;

    m_memoryStart = mmap(
        NULL,
        m_memoryLimit,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,            
        0           
    );

    if (m_memoryStart == MAP_FAILED) {
        m_memoryStart = nullptr;
        m_memoryLimit = 0;
        m_totalWords = 0;
        return;
    }

    m_blocks.clear();
    m_blocks.emplace_back(0, m_totalWords, true); // one big hole
}

void MemoryManager::shutdown() {
    if (m_memoryStart) {
        munmap(m_memoryStart, m_memoryLimit);
    }

    m_blocks.clear();
    m_memoryStart = nullptr;
    m_memoryLimit = 0;
    m_totalWords = 0;
}

void *MemoryManager::allocate(size_t sizeInBytes) {
    if (sizeInBytes == 0 || !m_memoryStart) {
        return nullptr;
    }

    size_t wordsNeeded = std::ceil(static_cast<double>(sizeInBytes) / m_wordSize);

    void *holeList = getList();
    if (!holeList) {
        return nullptr;
    }

    int offset = m_allocator(wordsNeeded, holeList);

    // delete the list created by getList()
    delete[] static_cast<uint8_t*>(holeList);

    if (offset == -1) {
        return nullptr;
    }

    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        if (it->is_hole && it->offset == static_cast<size_t>(offset)) {
            if (it->length == wordsNeeded) {
                it->is_hole = false;
            } else {
                // split the block 
                // create a new hole for the remainder
                size_t remainingLength = it->length - wordsNeeded;
                size_t newHoleOffset = it->offset + wordsNeeded;
                m_blocks.insert(std::next(it), MemoryBlock(newHoleOffset, remainingLength, true));
                
                // resize this block and mark it as allocated
                it->length = wordsNeeded;
                it->is_hole = false;
            }
            
            return static_cast<uint8_t*>(m_memoryStart) + (it->offset * m_wordSize);
        }
    }
    return nullptr;
}

void MemoryManager::free(void *address) {
    if (!address || !m_memoryStart) {
        return;
    }

    size_t byteOffset = static_cast<uint8_t*>(address) - static_cast<uint8_t*>(m_memoryStart);
    size_t wordOffset = byteOffset / m_wordSize;

    // find the allocated block
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        if (!it->is_hole && it->offset == wordOffset) {
            it->is_hole = true;
            
            mergeHoles();
            return;
        }
    }
}

void MemoryManager::mergeHoles() {
    auto it = m_blocks.begin();
    while (it != m_blocks.end()) {
        if (!it->is_hole) {
            ++it;
            continue;
        }

        auto next = std::next(it);
        if (next != m_blocks.end() && next->is_hole) {
            // if next block is also a hole, merge both blocks into one
            it->length += next->length;
            m_blocks.erase(next);
        } else {
            ++it;
        }
    }
}