#include <stdio.h>
#include <stdlib.h>
#include "ez_max_heap.h"

static const uint32_t ARRAY_SIZE = 50;
static const uint32_t HEAP_SIZE = 100;

#define CAST_UINT32_T(v)   (*(uint32_t*)(v))

static int compare_uint_data(HeapData orig, HeapData dest) {
    return CAST_UINT32_T(orig) > CAST_UINT32_T(dest) ? 1 : 0;
}

static void print_heap_data(HeapData data) {
    fprintf(stdout, "note: max heap's pop element is [%d].\n", CAST_UINT32_T(data));
}

int main(int argc, char **argv) {
    uint32_t array[ARRAY_SIZE];

    for (int i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = (uint32_t) (i + 1);
    }
    ezMaxHeap *heap = new_max_heap(HEAP_SIZE, &compare_uint_data);
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        push_max_heap(heap, array + i);
    }

    for (int i = ARRAY_SIZE - 1; i >= 0; --i) {
        HeapData data = pop_max_heap(heap);
        if (&array[i] != data) {
            fprintf(stdout, "测试错误了[%d]!=%d\n", i, CAST_UINT32_T(data));
            return EXIT_FAILURE;
        }
        print_heap_data(data);
    }

    // gdb>x /100hw heap+2
    free_max_heap(heap);

    return EXIT_SUCCESS;
}
