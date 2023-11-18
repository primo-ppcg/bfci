#include "bitarray.h"

BitArray bitarray_init() {
    BitArray bitarray = { .capacity = 8, .offset = 4, .bits = calloc(8, sizeof(uint32_t)) };
    return bitarray;
}

void bitarray_deinit(BitArray bitarray) {
    free(bitarray.bits);
}

static void ensure_capacity(BitArray *bitarray, int bit) {
    int loc = bitarray->offset + (bit >> 5);
    if(loc < 0) {
        bitarray->bits = realloc(bitarray->bits, (bitarray->capacity - loc) * sizeof(uint32_t));
        memmove(&bitarray->bits[-loc], bitarray->bits, bitarray->capacity * sizeof(uint32_t));
        memset(bitarray->bits, 0, (-loc) * sizeof(uint32_t));
        bitarray->capacity -= loc;
        bitarray->offset -= loc;
    } else if((unsigned)loc >= bitarray->capacity) {
        bitarray->bits = realloc(bitarray->bits, (loc + 1) * sizeof(uint32_t));
        memset(&bitarray->bits[bitarray->capacity], 0, (loc + 1 - bitarray->capacity) * sizeof(uint32_t));
        bitarray->capacity = loc + 1;
    }
}

bool get_bit(BitArray bitarray, int bit) {
    int loc = bitarray.offset + (bit >> 5);
    if(loc < 0 || (unsigned)loc >= bitarray.capacity) {
        return false;
    }
    return (bitarray.bits[loc] & (1 << (bit & 31))) > 0;
}

void set_bit(BitArray *bitarray, int bit) {
    ensure_capacity(bitarray, bit);
    int loc = bitarray->offset + (bit >> 5);
    bitarray->bits[loc] |= (1 << (bit & 31));
}

void unset_bit(BitArray *bitarray, int bit) {
    int loc = bitarray->offset + (bit >> 5);
    if(loc < 0 || (unsigned)loc >= bitarray->capacity) {
        return;
    }
    bitarray->bits[loc] &= ~(1 << (bit & 31));
}

bool toggle_bit(BitArray *bitarray, int bit) {
    ensure_capacity(bitarray, bit);
    int loc = bitarray->offset + (bit >> 5);
    return (bitarray->bits[loc] ^= (1 << (bit & 31))) > 0;
}
