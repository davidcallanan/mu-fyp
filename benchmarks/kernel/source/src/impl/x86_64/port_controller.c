#include "x86_64/port_controller.h"
#include "x86_64/port.h"

struct PortController port_controller__create() {
	struct PortController this;
	
	this.inb = port_inb;
	this.outb = port_outb;
	
	return this;
}
