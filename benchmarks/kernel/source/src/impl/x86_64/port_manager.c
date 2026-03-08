#include "x86_64/port_manager.h"
#include "x86_64/port_controller.h"
#include "kernel/heap.h"

// Programmable Interval Timer

#define PORT_PIT_CHANNEL0 0x40
#define PORT_PIT_COMMAND 0x43

static void pit_read(struct PortManager* this) {
	this->port_controller->outb(PORT_PIT_COMMAND, 0x00);
	this->port_controller->inb(PORT_PIT_CHANNEL0);
	this->port_controller->inb(PORT_PIT_CHANNEL0);
}

// Real-Time Clock

#define PORT_RTC_COMMAND 0x70
#define PORT_RTC_DATA 0x71
#define RTC_REGISTER_SECONDS 0x00
#define RTC_REGISTER_STATUS_A 0x0A
#define RTC_REGISTER_STATUS_B 0x0B
#define RTC_UPDATE_IN_PROGRESS 0x80
#define RTC_DATA_MODE (1 << 2)

static uint8_t rtc_read_register(struct PortManager* this, uint8_t reg) {
	this->port_controller->outb(PORT_RTC_COMMAND, reg);
	return this->port_controller->inb(PORT_RTC_DATA);
}

static void rtc_wait(struct PortManager* this) {
	while (rtc_read_register(this, RTC_REGISTER_STATUS_A) & RTC_UPDATE_IN_PROGRESS);
}

static uint8_t rtc_seconds(struct PortManager* this) {
	uint8_t seconds_a;
	uint8_t seconds_b;
	uint8_t is_bcd = !(rtc_read_register(this, RTC_REGISTER_STATUS_B) & RTC_DATA_MODE);

	do {
		rtc_wait(this);
		seconds_a = rtc_read_register(this, RTC_REGISTER_SECONDS);
		rtc_wait(this);
		seconds_b = rtc_read_register(this, RTC_REGISTER_SECONDS);
	} while (seconds_a != seconds_b);

	if (is_bcd) {
		return (seconds_b & 0x0F) + ((seconds_b & 0xF0) >> 4) * 10;
	}

	return seconds_b;
}

// IO Waiting

static void io_wait(struct PortManager* this) {
	this->port_controller->inb(0x80);
}

// VGA Stuff

#define PORT_VGA_CURSOR_COMMAND 0x3D4
#define PORT_VGA_CURSOR_DATA 0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F

static void vga_cursor_update(struct PortManager* this, uint16_t pos) {
	this->port_controller->outb(PORT_VGA_CURSOR_COMMAND, VGA_CURSOR_HIGH);
	this->port_controller->outb(PORT_VGA_CURSOR_DATA, (uint8_t)(pos >> 8));
	this->port_controller->outb(PORT_VGA_CURSOR_COMMAND, VGA_CURSOR_LOW);
	this->port_controller->outb(PORT_VGA_CURSOR_DATA, (uint8_t)(pos & 0xFF));
}

// Manager

struct PortManager* port_manager__create() {
	struct PortManager* this = dummy_alloc(sizeof(struct PortManager));
	
	struct PortController* port_controller = dummy_alloc(sizeof(struct PortController));
	*port_controller = port_controller__create();
	
	this->port_controller = port_controller;
	this->pit_read = pit_read;
	this->rtc_seconds = rtc_seconds;
	this->io_wait = io_wait;
	this->vga_cursor_update = vga_cursor_update;
	
	return this;
}
