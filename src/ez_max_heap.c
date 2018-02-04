#include "ez_max_heap.h"
#include "ez_malloc.h"

struct ezMaxHeap_t {
    uint32_t n;
    uint32_t a;
    HeapCmpFunc cmp;
    HeapData array[0]; // gnu c99 zero-size array
};

ezMaxHeap *new_max_heap(uint32_t size, HeapCmpFunc cmpFunc) {
    ezMaxHeap *heap = ez_malloc(sizeof(ezMaxHeap) + size * sizeof(HeapData));
    heap->n = 0;
    heap->a = size;
    heap->cmp = cmpFunc;
    return heap;
}

void free_max_heap(ezMaxHeap *heap) {
    ez_free(heap);
}

uint32_t max_heap_size(ezMaxHeap *heap) {
    return heap->n;
}

void push_max_heap(ezMaxHeap *heap, HeapData data) {
    int hole_index = heap->n++;
    int parent_index = (hole_index - 1) / 2;

    // 从 n 和 n的parent 比较，决定data的位置.
    while (hole_index > 0 && heap->cmp(heap->array[parent_index], data) < 1) {
        heap->array[hole_index] = heap->array[parent_index]; // parent down
        hole_index = parent_index;
        parent_index = (hole_index - 1) / 2;
    }
    heap->array[hole_index] = data;
}

HeapData pop_max_heap(ezMaxHeap *heap) {
    if (heap->n > 0) {
        HeapData data = heap->array[0];

        // 从 0 开始 决定[left, right]谁最大上来.
        int hole_index = 0;
        int min_child = 2 * (hole_index + 1); // 初始right.
        while (min_child <= heap->n) {
            // (left > right ? left : right)
            min_child -= (min_child == heap->n || heap->cmp(heap->array[min_child - 1], heap->array[min_child]) > 0) ? 1 : 0;
            heap->array[hole_index] = heap->array[min_child]; // child up
            hole_index = min_child;
            min_child = 2 * (hole_index + 1);
        }
        heap->array[min_child] = heap->array[--heap->n];

        return data;
    }
    return NULL;
}

