#include "x86_64/port_controller.h"
#include "x86_64/port.h"
#include "kernel/heap.h"

struct PortController* port_controller__create() {
	struct PortController* this = heap_alloc(sizeof(struct PortController));
	
	struct PortControllerVtable* vtable = heap_alloc(sizeof(struct PortControllerVtable));
	
	this->vtable = vtable;
	vtable->inb = port_inb;
	vtable->outb = port_outb;
	
	return this;
}
