#include "MemoryManager.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>

uint16_t read_u16(void* ptr) {
    uint16_t val;
    memcpy(&val, ptr, 2);
    return val;
}

int bestFit(int sizeInWords, void *list) {
    uint8_t* p = static_cast<uint8_t*>(list);
    uint16_t numHoles = read_u16(p);
    p += 2; 

    int bestOffset = -1;
    int minDiff = INT_MAX;

    for (uint16_t i = 0; i < numHoles; ++i) {
        uint16_t offset = read_u16(p);     
        uint16_t length = read_u16(p + 2); 
        p += 4;

        if (length >= sizeInWords) {
            int diff = length - sizeInWords;
            if (diff < minDiff) {
                minDiff = diff;
                bestOffset = offset;
            }
        }
    }
    return bestOffset;
}

int worstFit(int sizeInWords, void *list) {
    uint8_t* p = static_cast<uint8_t*>(list);
    uint16_t numHoles = read_u16(p);
    p += 2;

    int bestOffset = -1;
    int maxDiff = -1; 

    for (uint16_t i = 0; i < numHoles; ++i) {
        uint16_t offset = read_u16(p);
        uint16_t length = read_u16(p + 2);
        p += 4;

        if (length >= sizeInWords) {
            int diff = length - sizeInWords;
            if (diff > maxDiff) {
                maxDiff = diff;
                bestOffset = offset;
            }
        }
    }
    return bestOffset;
}

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
    : m_wordSize(wordSize), m_allocator(allocator), m_totalWords(0),
      m_memoryLimit(0), m_memoryStart(nullptr) {
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    m_allocator = allocator;
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
    m_blocks.emplace_back(0, m_totalWords, true);
}

void *MemoryManager::getBitmap() {
    if (!m_memoryStart) {
        return nullptr;
    }

    uint16_t bitmapByteSize = static_cast<uint16_t>(std::ceil(static_cast<double>(m_totalWords) / 8.0));
    
    size_t totalArraySize = 2 + bitmapByteSize;
    uint8_t* data = new uint8_t[totalArraySize];

    memset(data, 0, totalArraySize);
    memcpy(data, &bitmapByteSize, 2);

    uint8_t* bitmap = data + 2; 

    for (const auto& block : m_blocks) {
        if (!block.is_hole) { 
            for (size_t i = 0; i < block.length; ++i) {
                size_t wordIndex = block.offset + i;
                
                size_t byteIndex = wordIndex / 8;
                int bitPosition = wordIndex % 8;

                if (byteIndex < bitmapByteSize) { // Safety bounds check
                    bitmap[byteIndex] |= (1 << bitPosition);
                }
            }
        }
    }

    return data;
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

    delete[] static_cast<uint8_t*>(holeList);

    if (offset == -1) {
        return nullptr;
    }

    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        if (it->is_hole && it->offset == static_cast<size_t>(offset)) {
            if (it->length == wordsNeeded) {
                it->is_hole = false;
            } else {
                size_t remainingLength = it->length - wordsNeeded;
                size_t newHoleOffset = it->offset + wordsNeeded;
                m_blocks.insert(std::next(it), MemoryBlock(newHoleOffset, remainingLength, true));
                
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

void *MemoryManager::getList() {
    if (!m_memoryStart) {
        return nullptr;
    }

    uint16_t holeCount = 0;
    for (const auto& block : m_blocks) {
        if (block.is_hole) {
            holeCount++;
        }
    }

    if (holeCount == 0) {
        return nullptr;
    }

    // 2 bytes for count + (holes * 4 bytes/hole)
    size_t arraySize = 2 + (holeCount * 4);
    uint8_t* data = new uint8_t[arraySize];

    memcpy(data, &holeCount, 2);

    uint8_t* current = data + 2; // start writing after the 2-byte count
    for (const auto& block : m_blocks) {
        if (block.is_hole) {
            uint16_t offset = static_cast<uint16_t>(block.offset);
            uint16_t length = static_cast<uint16_t>(block.length);

            memcpy(current, &offset, 2);
            memcpy(current + 2, &length, 2);
            current += 4;
        }
    }
    return data;
}

int MemoryManager::dumpMemoryMap(char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return -1;
    }

    std::stringstream ss;
    bool first = true;
    for (const auto& block : m_blocks) {
        if (block.is_hole) {
            if (!first) {
                ss << " ";
            }
            ss << "[" << block.offset << ", " << block.length << "]";
            first = false;
        }
    }
    std::string mapString = ss.str();

    ssize_t bytesWritten = write(fd, mapString.c_str(), mapString.length());
    
    close(fd);

    if (bytesWritten != static_cast<ssize_t>(mapString.length())) {
        return -1;
    }

    return 0;
}