#include "kernel/heap.h"

#define HEAP_SIZE 65536

static char heap[HEAP_SIZE];
static size_t heap_top = 0;

void* dummy_alloc(size_t size) {
	void* ptr = &heap[heap_top];
	
	heap_top += size;
	
	return ptr;
}
