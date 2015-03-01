#ifndef _MIN_HEAP_
#define _MIN_HEAP_

typedef void *gpointer;

#define  MIN_HEAP_NOT_FUND          -1

typedef int cmpHandler(gpointer pv, gpointer nv);
typedef int findCmpHandler(gpointer args, gpointer data);
typedef void freeValueHandler(gpointer v);

typedef struct ezMinHeap_t ezMinHeap;
struct ezMinHeap_t {
	gpointer *array;
	int n;			/* array length */
	int a;			/* array alloc length */
	cmpHandler *heap_comp_func;
	freeValueHandler *heap_free_func;
};

void ez_min_heap_init(ezMinHeap *m, cmpHandler * handler, freeValueHandler * free_handler);

void ez_min_heap_free(ezMinHeap * m);

int ez_min_heap_size(ezMinHeap * m);

int ez_min_heap_push(ezMinHeap * m, gpointer v);

void *ez_min_heap_pop(ezMinHeap * m);

void *ez_min_heap_min(ezMinHeap * m);

int ez_min_heap_find(ezMinHeap * m, findCmpHandler *find_cmp_proc, gpointer cmp_args);

gpointer ez_min_heap_delete(ezMinHeap * m, int index);

#endif				/* _MIN_HEAP_ */
