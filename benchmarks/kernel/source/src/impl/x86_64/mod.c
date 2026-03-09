#include "x86_64/mod.h"
#include "x86_64/port_manager.h"
#include "kernel/heap.h"

struct Mod* mod__create() {
	struct Mod* this = heap_alloc(sizeof(struct Mod));
	
	this->port_manager = port_manager__create();
	
	return this;
}
