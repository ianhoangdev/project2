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