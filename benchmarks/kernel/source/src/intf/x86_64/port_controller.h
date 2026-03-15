#pragma once

#include <stdint.h>

// struct PortControllerVtable {
// 	uint8_t (*inb)(uint16_t port);
// 	void (*outb)(uint16_t port, uint8_t data);
// };

struct PortController {
	// struct PortControllerVtable* vtable;
};

struct PortController* port_controller__create();

uint8_t port_controller__inb(struct PortController* this, uint16_t port);
void port_controller__outb(struct PortController* this, uint16_t port, uint8_t data);
