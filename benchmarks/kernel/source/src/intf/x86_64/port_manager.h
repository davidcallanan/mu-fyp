#pragma once

#include <stdint.h>
#include "x86_64/port_controller.h"

struct PortManager;

// struct PortManagerVtable {
// 	// this is roughly what a vtable implementation would look like, to allow runtime polymorphism.
// 	// in C++ one would just use the "virtual" keyword to achieve the same.
// 	void (*pit_read)(struct PortManager* this);
// 	uint8_t (*rtc_seconds)(struct PortManager* this);
// 	void (*io_wait)(struct PortManager* this);
// 	void (*vga_cursor_update)(struct PortManager* this, uint16_t pos);
// };

struct PortManager {
	// use of a pointer simulates being able to substitute the implementation at runtime.
	struct PortController* port_controller;
	
	// use of a vtable enables dynamic dispatch.
	struct PortManagerVtable* vtable;
};

// pointer to implementation - allows substituting the data.
// vtable - allows substituting the logic.
// (really these are one and the same).

struct PortManager* port_manager__create();

void port_manager__pit_read(struct PortManager* this);
uint8_t port_manager__rtc_seconds(struct PortManager* this);
void port_manager__io_wait(struct PortManager* this);
void port_manager__vga_cursor_update(struct PortManager* this, uint16_t pos);
