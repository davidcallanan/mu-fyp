#include "x86_64/port_controller.h"
#include "x86_64/port.h"
#include "kernel/heap.h"

struct PortController* port_controller__create() {
	struct PortController* this = heap_alloc(sizeof(struct PortController));
	
	// struct PortControllerVtable* vtable = heap_alloc(sizeof(struct PortControllerVtable));
	
	// this->vtable = vtable;
	// vtable->inb = port_inb;
	// vtable->outb = port_outb;
	
	return this;
}

uint8_t port_controller__inb(struct PortController* this, uint16_t port) {
	(void)this;
	return port_inb(port);
}

void port_controller__outb(struct PortController* this, uint16_t port, uint8_t data) {
	(void)this;
	port_outb(port, data);
}
