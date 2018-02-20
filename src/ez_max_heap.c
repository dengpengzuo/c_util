#include "ez_max_heap.h"
#include "ez_malloc.h"

struct ezMaxHeap_t {
    uint32_t n;
    uint32_t a;
    HeapCmpFunc cmp;
    HeapData array[0]; // gnu c99 zero-size array
};

static ezMaxHeap *extend_max_heap(ezMaxHeap *heap) {
    size_t new_size = sizeof(ezMaxHeap) + (heap->a + 16) * sizeof(HeapData);
    ezMaxHeap *newHeap = ez_realloc(heap, new_size);
    newHeap->a += 16;
    return newHeap;
}

ezMaxHeap *new_max_heap(uint32_t size, HeapCmpFunc cmpFunc) {
    ezMaxHeap *heap = ez_malloc(sizeof(ezMaxHeap) + size * sizeof(HeapData));
    heap->a = size;
    heap->n = 0;
    heap->cmp = cmpFunc;
    return heap;
}

void free_max_heap(ezMaxHeap *heap) {
    ez_free(heap);
}

uint32_t max_heap_size(ezMaxHeap *heap) {
    return heap->n;
}

ezMaxHeap *push_max_heap(ezMaxHeap *heap, HeapData data) {
    if (heap->n >= heap->a) {
        heap = extend_max_heap(heap);
    }
    int hole_index = heap->n++;
    int parent_index = (hole_index - 1) / 2;

    // 从 n 和 n的parent 比较，决定data的位置.
    while (hole_index > 0 && heap->cmp(heap->array[parent_index], data) < 1) {
        heap->array[hole_index] = heap->array[parent_index]; // parent down
        hole_index = parent_index;
        parent_index = (hole_index - 1) / 2;
    }
    heap->array[hole_index] = data;
    return heap;
}

HeapData pop_max_heap(ezMaxHeap *heap) {
    if (heap->n > 0) {
        HeapData data = heap->array[0];

        // 从 0 开始 决定child中[left, right]谁最大，最大上来，继续儿子的儿子.
        int hole_index = 0;
        int min_child = 2 * (hole_index + 1); // 初始right.
        while (min_child <= heap->n) {
            // (left > right ? left : right)
            min_child -= (min_child == heap->n || heap->cmp(heap->array[min_child - 1], heap->array[min_child]) > 0) ? 1
                                                                                                                     : 0;
            heap->array[hole_index] = heap->array[min_child]; // child up
            hole_index = min_child;
            min_child = 2 * (hole_index + 1);
        }
        heap->array[hole_index] = heap->array[--heap->n];

        return data;
    }
    return NULL;
}

