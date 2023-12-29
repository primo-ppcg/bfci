#include "annotator.h"
#include "bitarray.h"

static uint8_t mod_inv(uint16_t a) {
    uint16_t x = 0;
    uint16_t u = 1;
    uint16_t n = 256;
    while(a != 0) {
        uint16_t tmpx = x;
        x = u;
        u = tmpx - (n / a) * u;
        uint16_t tmpa = a;
        a = n % a;
        n = tmpa;
    }
    return (uint8_t)(x & 0xFF);
}

Annotation annotation_init() {
    Annotation annotation = {
        .elidable = false,
        .value = 0,
        .children = 0,
        .capacity = 16,
        .child_values = malloc(16 * sizeof(uint8_t))
    };
    return annotation;
}

void annotation_deinit(Annotation annotation) {
    free(annotation.child_values);
}

void annotation_append(Annotation *annotation, uint8_t child_value) {
    if(annotation->children + 1 > annotation->capacity) {
        annotation->capacity += 16;
        annotation->child_values = realloc(annotation->child_values, annotation->capacity * sizeof(uint8_t));
    }
    annotation->child_values[annotation->children] = child_value;
    annotation->children++;
}

Annotation annotate(char *source, size_t srclen, size_t i) {
    Annotation annotation = annotation_init();
    BitArray dependancies = bitarray_init();

    int depth = 0;
    int16_t inner_shift = 0;
    int16_t total_shift = 0;
    uint8_t base = 0;
    uint8_t inner_base = 0;
    bool has_dependants = false;
    bool inner_has_dependants = false;

    for(; i < srclen; i++) {
        char c = source[i];
        switch(c) {
            case '<':
            case '>':
                inner_shift += c - 61;
                total_shift += c - 61;
                break;
            case '+':
            case '-':
                if(get_bit(dependancies, total_shift)) {
                    // not elidable, a dependancy has been modified
                    bitarray_deinit(dependancies);
                    return annotation;
                } else if(total_shift == 0) {
                    if(depth > 0) {
                        // not elidable, the outer dependancy has been modified
                        bitarray_deinit(dependancies);
                        return annotation;
                    } else {
                        base += 44 - c;
                    }
                } else if(depth > 0) {
                    if(inner_shift == 0) {
                        inner_base += 44 - c;
                    } else {
                        has_dependants = true;
                        inner_has_dependants = true;
                    }
                } else {
                    has_dependants = true;
                }
                break;
            case '.':
            case ',':
                return annotation;
            case '[':
                if(depth > 0) {
                    // not elidable, only unroll to a depth of 1 (for now?)
                    bitarray_deinit(dependancies);
                    return annotation;
                }
                depth++;
                inner_shift = 0;
                inner_base = 0;
                inner_has_dependants = false;
                break;
            case ']':
                if(depth == 0) {
                    // elidable only if balanced, and has a proper inverse mod 256 (i.e. not even)
                    annotation.elidable = total_shift == 0 && (base & 1) == 1;
                    annotation.value = has_dependants ? mod_inv(256 - (uint16_t)base) : 0;
                    bitarray_deinit(dependancies);
                    return annotation;
                } else {
                    if(inner_shift != 0 || (inner_base & 1) != 1) {
                        // not elidable, unbalanced inner loop
                        bitarray_deinit(dependancies);
                        return annotation;
                    }
                    if(inner_has_dependants) {
                        set_bit(&dependancies, total_shift);
                        annotation_append(&annotation, mod_inv(256 - (uint16_t)inner_base));
                    } else {
                        // no dependants (i.e. [-])
                        annotation_append(&annotation, 0);
                    }
                    depth--;
                }
                break;
            default:
                break;
        }
    }

    bitarray_deinit(dependancies);
    return annotation;
}
