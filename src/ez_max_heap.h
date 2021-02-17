#pragma once

#include <stdint.h>

typedef void* heap_data;
typedef struct ez_max_heap_s ez_max_heap_t;

typedef int (*HeapCmpFunc)(heap_data orig, heap_data dest);

ez_max_heap_t* new_max_heap(uint32_t size, HeapCmpFunc cmpFunc);

void free_max_heap(ez_max_heap_t* heap);

uint32_t max_heap_size(ez_max_heap_t* heap);

ez_max_heap_t* push_max_heap(ez_max_heap_t* heap, heap_data data);

heap_data pop_max_heap(ez_max_heap_t* heap);
