#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    bool elidable;
    uint8_t value;
    size_t children;
    size_t capacity;
    uint8_t *child_values;
} Annotation;

Annotation annotation_init();
void annotation_deinit(Annotation annotation);

void annotation_append(Annotation *annotation, uint8_t child_value);
Annotation annotate(char *source, size_t srclen, size_t i);
