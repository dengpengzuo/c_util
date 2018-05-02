#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "ez_string.h"
#include "ez_util.h"
#include "ez_log.h"
#include "ez_malloc.h"
#include "ez_max_heap.h"

#define ngx_int_t  int32_t
#define ngx_uint_t uint32_t

extern char **environ;
static int ngx_os_argc;
static char **ngx_os_argv;
static char *ngx_os_argv_last;

ngx_int_t ngx_init_setproctitle() {
    char *p;
    size_t size;
    ngx_uint_t i;

    size = 0;

    for (i = 0; environ[i]; i++) {
        size += strlen(environ[i]) + 1;
    }

    p = ez_malloc(size);
    if (p == NULL) {
        return 1;
    }

    ngx_os_argv_last = ngx_os_argv[0];

    for (i = 0; i < ngx_os_argc; i++) {
        if (ngx_os_argv_last == ngx_os_argv[i]) {
            ngx_os_argv_last = ngx_os_argv[i] + strlen(ngx_os_argv[i]) + 1;
        }
    }

    for (i = 0; environ[i]; i++) {
        if (ngx_os_argv_last == environ[i]) {

            size = strlen(environ[i]) + 1;
            ngx_os_argv_last = environ[i] + size;

            ez_strncpy(p, environ[i], size);
            environ[i] = (char *) p;
            p += size;
        }
    }

    ngx_os_argv_last--; // 这是 argv 能操作的最大值.

    return 0;
}

void ngx_setproctitle(const char *new_titles) {
    ngx_os_argv[1] = NULL;
    char *p;
    p = ez_strncpy(ngx_os_argv[0], "myheap:", ngx_os_argv_last - ngx_os_argv[0]);
    p = ez_strncpy(p, (char *) new_titles, ngx_os_argv_last - p);

    if (ngx_os_argv_last - p) {
        memset(p, '\0', ngx_os_argv_last - p);
    }
}

void test_utf8() {
    // utf8.bytes
    uint8_t buf[128];
    size_t r = ez_read_file("./test/hello.txt", buf, 128);
    buf[r] = '\0';
    log_hexdump(LOG_INFO, buf, r, "utf8");

    // utf8-> unicode
    wchar_t dst[128];
    r = ez_utf8_decode(dst, buf, r);
    dst[r] = '\0';
    log_hexdump(LOG_INFO, (uint8_t*) &dst[0], r * 2, "unicode");
}

static const uint32_t ARRAY_SIZE = 50;
#define CAST_UINT32_T(v)   (*(uint32_t*)(v))

static int compare_uint_data(heap_data orig, heap_data dest) {
    return CAST_UINT32_T(orig) > CAST_UINT32_T(dest) ? 1 : 0;
}

static void print_heap_data(heap_data data) {
    fprintf(stdout, "note: max heap's pop element is [%d].\n", CAST_UINT32_T(data));
}

int main(int argc, char **argv) {
    ngx_os_argc = argc;
    ngx_os_argv = argv;

    // linux 内在部局为: << argv | environ >>
    ngx_init_setproctitle();
    ngx_setproctitle("myworker; ssss : bbbbbbb");

    log_init(LOG_VVERB, NULL);

    test_utf8();

    uint32_t array[ARRAY_SIZE];

    for (int i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = (uint32_t) (i + 1);
    }
    ez_max_heap_t *heap = new_max_heap(10, &compare_uint_data);
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        heap = push_max_heap(heap, array + i); // 内部有扩容，地址会发生变化，外面需要跟进这个内存变化.
    }

    for (int i = ARRAY_SIZE - 1; i >= 0; --i) {
        heap_data data = pop_max_heap(heap);
        if (&array[i] != data) {
            fprintf(stdout, "测试错误了[%d]!=%d\n", array[i], CAST_UINT32_T(data));
            return EXIT_FAILURE;
        }
        if (i != max_heap_size(heap)) {
            fprintf(stdout, "测试错误了[%d]!=%d\n", i, max_heap_size(heap));
            return EXIT_FAILURE;
        }
        print_heap_data(data);
    }

    // gdb>x /100hw heap+2
    free_max_heap(heap);

    return EXIT_SUCCESS;
}
