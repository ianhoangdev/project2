#include "MemoryManager.h"
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sstream>
#include <cmath>
#include <vector>
#include <string>
#include <utility>

using namespace std;

MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator)
    : wordSize(wordSize),
      allocator(allocator),
      memoryStart(nullptr),
      alignedStart(nullptr),
      memoryLimit(0),
      headNode(nullptr) {}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    
}

void MemoryManager::shutdown() {

}

void *MemoryManager::allocate(size_t sizeInBytes) {

}

void MemoryManager::free(void *address) {}

void MemoryManager::setAllocator(function<int(int, void *)> allocator) {}

int MemoryManager::dumpMemoryMap(char *filename) {}

void *MemoryManager::getList() {}

void *MemoryManager::getBitmap() {}

unsigned MemoryManager::getWordSize() {
    return wordSize;
}

void *MemoryManager::getMemoryStart() {
    return alignedStart;
}

unsigned MemoryManager::getMemoryLimit() {
    return memoryLimit;
}

size_t MemoryManager::align(size_t size) {}

void MemoryManager::mergeNext(MemoryBlock *block) {}

void MemoryManager::splitBlock(MemoryBlock *block, size_t requiredSize) {}

int bestFit(int sizeInWords, void *list) {}

int worstFit(int sizeInWords, void *list) {}