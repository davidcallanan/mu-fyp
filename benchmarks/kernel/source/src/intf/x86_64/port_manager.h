#pragma once

#include <stdint.h>
#include "x86_64/port_controller.h"

struct PortManager {
	struct PortController* port_controller;
};

struct PortManager* port_manager__create();

void port_manager__pit_read(struct PortManager* this);
uint8_t port_manager__rtc_seconds(struct PortManager* this);
void port_manager__io_wait(struct PortManager* this);
void port_manager__vga_cursor_update(struct PortManager* this, uint16_t pos);
