#pragma once

#include <stdint.h>
#include "x86_64/port_controller.h"

struct PortManager {
	// use of a pointer simulates being able to substitute the implementation at runtime.
	struct PortController* port_controller;
	
	// this simulates a vtable.
	void (*pit_read)(struct PortManager* this);
	uint8_t (*rtc_seconds)(struct PortManager* this);
	void (*io_wait)(struct PortManager* this);
	void (*vga_cursor_update)(struct PortManager* this, uint16_t pos);
};

struct PortManager* port_manager__create();
