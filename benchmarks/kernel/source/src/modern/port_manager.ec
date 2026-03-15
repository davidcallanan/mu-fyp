; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

type PortManager {};

; type PortManagerRef (&mut PortManager);

@PortManager:port_controller PortController;

; ; PIT constants
; @PortManager:port_pit_channel0 u16 0x40;
; @PortManager:port_pit_command u16 0x43;

; ; RTC constants
; @PortManager:port_rtc_command u16 0x70;
; @PortManager:port_rtc_data u16 0x71;
; @PortManager:rtc_register_seconds u8 0x00;
; @PortManager:rtc_register_status_a u8 0x0A;
; @PortManager:rtc_register_status_b u8 0x0B;
; @PortManager:rtc_update_in_progress u8 0x80;
; @PortManager:rtc_data_mode u8 4;

; ; VGA cursor constants
; @PortManager:port_vga_cursor_command u16 0x3D4;
; @PortManager:port_vga_cursor_data u16 0x3D5;
; @PortManager:vga_cursor_high u8 0x0E;
; @PortManager:vga_cursor_low u8 0x0F;

@PortManager:pit_read input {} -> {
	this:port_controller:outb(u16 0x43, u8 0x00); PORT_PIT_COMMAND=0x43
	this:port_controller:inb(u16 0x40); PORT_PIT_CHANNEL0=0x40
	this:port_controller:inb(u16 0x40); PORT_PIT_CHANNEL0=0x40
};

@PortManager:rtc_read_register input {
	:reg u8;
} -> U8Result {
	this:port_controller:outb(u16 0x70, input:reg); PORT_RTC_COMMAND=0x70
	result := this:port_controller:inb(u16 0x71):0; PORT_RTC_DATA=0x71
	:0 result;
};

@PortManager:rtc_wait input {} -> {
	that := &PortManager this;
	
	for {
		status := that:rtc_read_register(u8 0x0A):0; RTC_REGISTER_STATUS_A=0x0A
		still_updating := status b& u8 0x80; RTC_UPDATE_IN_PROGRESS=0x80
		if (still_updating == u8 0) {
			break;
		}
	}
};

@PortManager:rtc_seconds input {} -> U8Result {
	that := &PortManager this;
	
	is_bcd_raw := that:rtc_read_register(u8 0x0B):0; RTC_REGISTER_STATUS_B=0x0B
	has_data_mode := is_bcd_raw b& u8 4; RTC_DATA_MODE=4
	is_bcd := has_data_mode == u8 0;

	mut seconds_a := u8 0;
	mut seconds_b := u8 0;

	for {
		that:rtc_wait {};
		seconds_a = that:rtc_read_register(u8 0x00):0; RTC_REGISTER_SECONDS=0x00
		that:rtc_wait {};
		seconds_b = that:rtc_read_register(u8 0x00):0; RTC_REGISTER_SECONDS=0x00
		
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
	this:port_controller:inb(u16 0x80);
};

@PortManager:vga_cursor_update input {
	:pos u16;
} -> {
	pos_high := u8 input:pos >> u16 8;
	pos_low := u8 input:pos b& u16 0xFF;
	this:port_controller:outb(u16 0x3D4, u8 0x0E); PORT_VGA_CURSOR_COMMAND=0x3D4, VGA_CURSOR_HIGH=0x0E
	this:port_controller:outb(u16 0x3D5, pos_high); PORT_VGA_CURSOR_DATA=0x3D5
	this:port_controller:outb(u16 0x3D4, u8 0x0F); PORT_VGA_CURSOR_COMMAND=0x3D4, VGA_CURSOR_LOW=0x0F
	this:port_controller:outb(u16 0x3D5, pos_low); PORT_VGA_CURSOR_DATA=0x3D5
};

@Mod:port_manager_create input {} -> PortManager {
	:port_controller mod:port_controller_create {};
	; raw := mod:heap_alloc(sizeof(PortManager)):0;
	; pm := &mut PortManager nullptr;
	; pm:port_controller = mod:port_controller_create {};
	; :0 pm;
};
