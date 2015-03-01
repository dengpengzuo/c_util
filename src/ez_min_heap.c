#include "ez_malloc.h"
#include "ez_min_heap.h"

#define  MIN_HEAP_SIZE      1024

static int ez_min_heap_resize_(ezMinHeap * m);

static void ez_min_heap_shift_up_(ezMinHeap * m, int hole_index, gpointer v);

static void ez_min_heap_shift_down_(ezMinHeap * m, int hole_index, gpointer v);

void ez_min_heap_init(ezMinHeap * m, cmpHandler * cmp_handler, freeValueHandler * free_handler)
{
	m->array = ez_malloc(sizeof(gpointer) * MIN_HEAP_SIZE);
	m->a = MIN_HEAP_SIZE;
	m->n = 0;
	m->heap_comp_func = cmp_handler;
	m->heap_free_func = free_handler;
}

void ez_min_heap_free(ezMinHeap * m)
{
	// 调用heap中对象的release挂勾.
	if (m->heap_free_func != NULL) {
		for (int i = 0; i < m->n; ++i)
			if (m->array[i] != NULL)
				m->heap_free_func(m->array[i]);
	}
	ez_free(m->array);
}

int ez_min_heap_size(ezMinHeap * m)
{
	return m->n;
}

int ez_min_heap_push(ezMinHeap * m, gpointer v)
{
	if ((m->n == m->a) && ez_min_heap_resize_(m)) {
		return -1;
	}
	ez_min_heap_shift_up_(m, m->n++, v);
	return 0;
}

gpointer ez_min_heap_pop(ezMinHeap * m)
{
	if (m->n == 0)
		return NULL;
	gpointer v = m->array[0];
	ez_min_heap_shift_down_(m, 0, m->array[--(m->n)]);
	m->array[m->n] = 0;
	return v;
}

gpointer ez_min_heap_min(ezMinHeap * m)
{
	if (m->n == 0)
		return NULL;
	return m->array[0];
}

int ez_min_heap_find(ezMinHeap * m, findCmpHandler * find_cmp_proc, gpointer args)
{
	int i = 0, r = 0;
	if (m->n == 0)
		return MIN_HEAP_NOT_FUND;

	for (; i < m->n; ++i) {
		r = find_cmp_proc(args, m->array[i]);
		if (r)
			break;
	}
	return r ? i : MIN_HEAP_NOT_FUND;
}

gpointer ez_min_heap_delete(ezMinHeap * m, int min_heap_idx)
{
	gpointer last, del;
	if (m->n == 0 || min_heap_idx < 0 || min_heap_idx >= m->n)
		return NULL;

	last = m->array[--m->n];
	del = m->array[min_heap_idx];

	int parent = (min_heap_idx - 1) / 2;
	if (min_heap_idx > 0 && m->heap_comp_func(m->array[parent], last))
		ez_min_heap_shift_up_(m, min_heap_idx, last);
	else
		ez_min_heap_shift_down_(m, min_heap_idx, last);

	return del;
}

int ez_min_heap_resize_(ezMinHeap * m)
{
	int new_a = m->a << 1;
	void *na = ez_realloc(m->array, sizeof(gpointer) * new_a);
	if (na == NULL)
		return -1;
	m->array = na;
	m->a = new_a;
	return 0;
}

void ez_min_heap_shift_up_(ezMinHeap * m, int hole_index, gpointer v)
{
	int parent = (hole_index - 1) / 2;
	while (hole_index > 0 && m->heap_comp_func(m->array[parent], v)) {
		m->array[hole_index] = m->array[parent];
		hole_index = parent;
		parent = (hole_index - 1) / 2;
	}
	m->array[hole_index] = v;
}

void ez_min_heap_shift_down_(ezMinHeap * m, int hole_index, gpointer last)
{
	int min_child = 2 * (hole_index + 1);
	while (min_child <= m->n) {
		min_child -=
		    m->heap_comp_func(m->array[min_child], m->array[min_child - 1]) ? 1 : 0;
		if (!m->heap_comp_func(last, m->array[min_child]))
			break;
		m->array[hole_index] = m->array[min_child];
		hole_index = min_child;
		min_child = 2 * (hole_index + 1);
	}
	ez_min_heap_shift_up_(m, hole_index, last);
}
