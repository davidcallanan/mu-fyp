#pragma once

#include <stdint.h>

struct PortControllerVtable {
	uint8_t (*inb)(uint16_t port);
	void (*outb)(uint16_t port, uint8_t data);
};

struct PortController {
	struct PortControllerVtable* vtable;
};

struct PortController* port_controller__create();
