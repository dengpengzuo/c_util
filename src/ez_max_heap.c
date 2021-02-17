#include "ez_max_heap.h"

#include "ez_malloc.h"

struct ez_max_heap_s {
    uint32_t n;
    uint32_t a;
    HeapCmpFunc cmp;
    heap_data array[0]; // gnu c99 zero-size array
};

static ez_max_heap_t* extend_max_heap(ez_max_heap_t* heap)
{
    size_t new_size = sizeof(ez_max_heap_t) + (heap->a + 16) * sizeof(heap_data);
    ez_max_heap_t* newHeap = ez_realloc(heap, new_size);
    newHeap->a += 16;
    return newHeap;
}

ez_max_heap_t* new_max_heap(uint32_t size, HeapCmpFunc cmpFunc)
{
    ez_max_heap_t* heap = ez_malloc(sizeof(ez_max_heap_t) + size * sizeof(heap_data));
    heap->a = size;
    heap->n = 0;
    heap->cmp = cmpFunc;
    return heap;
}

void free_max_heap(ez_max_heap_t* heap)
{
    ez_free(heap);
}

uint32_t max_heap_size(ez_max_heap_t* heap)
{
    return heap->n;
}

ez_max_heap_t* push_max_heap(ez_max_heap_t* heap, heap_data data)
{
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

heap_data pop_max_heap(ez_max_heap_t* heap)
{
    if (heap->n > 0) {
        heap_data data = heap->array[0];

        // 从 0 开始 决定child中[left, right]谁最大，最大上来，继续儿子的儿子.
        int hole_index = 0;
        int min_child = 2 * (hole_index + 1); // 初始right.
        while (min_child <= heap->n) {
            // (left > right ? left : right)
            min_child -= (min_child == heap->n || heap->cmp(heap->array[min_child - 1], heap->array[min_child]) > 0) ? 1 : 0;
            heap->array[hole_index] = heap->array[min_child]; // child up
            hole_index = min_child;
            min_child = 2 * (hole_index + 1);
        }
        heap->array[hole_index] = heap->array[--heap->n];

        return data;
    }
    return NULL;
}
