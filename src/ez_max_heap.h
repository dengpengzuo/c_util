#pragma once

#include <stdint.h>

typedef void *HeapData;
typedef struct ezMaxHeap_t ezMaxHeap;

typedef int (*HeapCmpFunc)(HeapData orig, HeapData dest);

ezMaxHeap *new_max_heap(uint32_t size, HeapCmpFunc cmpFunc);

uint32_t max_heap_size(ezMaxHeap *heap);

void free_max_heap(ezMaxHeap *heap);

ezMaxHeap *push_max_heap(ezMaxHeap *heap, HeapData data);

HeapData pop_max_heap(ezMaxHeap *heap);
