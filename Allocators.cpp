#include <cstdint>  
#include <cstring>  
#include <climits>  

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