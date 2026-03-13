; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

type PortManager {};

; PIT constants
@PortManager:port_pit_channel0 u16 0x40;
@PortManager:port_pit_command u16 0x43;

; RTC constants
@PortManager:port_rtc_command u16 0x70;
@PortManager:port_rtc_data u16 0x71;
@PortManager:rtc_register_seconds u8 0x00;
@PortManager:rtc_register_status_a u8 0x0A;
@PortManager:rtc_register_status_b u8 0x0B;
@PortManager:rtc_update_in_progress u8 0x80;
@PortManager:rtc_data_mode u8 4;

; VGA cursor constants
@PortManager:port_vga_cursor_command u16 0x3D4;
@PortManager:port_vga_cursor_data u16 0x3D5;
@PortManager:vga_cursor_high u8 0x0E;
@PortManager:vga_cursor_low u8 0x0F;

@PortManager:pit_read input {} -> {
	this:outb(this:port_pit_command, u8 0x00);
	this:inb(this:port_pit_channel0);
	this:inb(this:port_pit_channel0);
};

@PortManager:rtc_read_register input {
	:reg u8;
} -> U8Result {
	this:outb(this:port_rtc_command, input:reg);
	:0 this:inb(this:port_rtc_data):0;
};

@PortManager:rtc_wait input {} -> {
	for {
		status := this:rtc_read_register(this:rtc_register_status_a):0;
		still_updating := status b& this:rtc_update_in_progress;
		if (still_updating == u8 0) {
			break;
		}
	}
};

@PortManager:rtc_seconds input {} -> U8Result {
	is_bcd_raw := this:rtc_read_register(this:rtc_register_status_b):0;
	has_data_mode := is_bcd_raw b& this:rtc_data_mode;
	is_bcd := has_data_mode == u8 0;

	mut seconds_a := u8 0;
	mut seconds_b := u8 0;

	for {
		this:rtc_wait {};
		seconds_a = this:rtc_read_register(this:rtc_register_seconds):0;
		this:rtc_wait {};
		seconds_b = this:rtc_read_register(this:rtc_register_seconds):0;

		if (seconds_a == seconds_b) {
			break;
		}
	}

	mut result := u8 0;

	if (is_bcd) {
		low := seconds_b b& u8 0x0F;
		high_raw := seconds_b b& u8 0xF0;
		high := high_raw >> u8 4;
		result = low + high * u8 10;
	} else {
		result = seconds_b;
	}

	:0 result;
};

@PortManager:io_wait input {} -> {
	this:inb(u16 0x80);
};

@PortManager:vga_cursor_update input {
	:pos u16;
} -> {
	pos_high := u8 input:pos >> u16 8;
	pos_low := u8 input:pos b& u16 0xFF;
	this:outb(this:port_vga_cursor_command, this:vga_cursor_high);
	this:outb(this:port_vga_cursor_data, pos_high);
	this:outb(this:port_vga_cursor_command, this:vga_cursor_low);
	this:outb(this:port_vga_cursor_data, pos_low);
};

; Delegate port I/O to the underlying PortController

@PortManager:inb input {
	:port u16;
} -> U8Result {
	pc := mod:port_controller_create {};
	:0 pc:inb(input:port):0;
};

@PortManager:outb input {
	:port u16;
	:data u8;
} -> {
	pc := mod:port_controller_create {};
	pc:outb(input:port, input:data);
};

@Mod:port_manager_create input {} -> PortManager {
};
