#pragma once

#include <stdint.h>

struct PortController {
	// this simulates a vtable.
	uint8_t (*inb)(uint16_t port);
	void (*outb)(uint16_t port, uint8_t data);
};

struct PortController port_controller__create();
