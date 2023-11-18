#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    size_t capacity;
    size_t offset;
    uint32_t *bits;
} BitArray;

BitArray bitarray_init();
void bitarray_deinit(BitArray bitarray);

bool get_bit(BitArray bitarray, int bit);
void set_bit(BitArray *bitarray, int bit);
void unset_bit(BitArray *bitarray, int bit);
bool toggle_bit(BitArray *bitarray, int bit);
