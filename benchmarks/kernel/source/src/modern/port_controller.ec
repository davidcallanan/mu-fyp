; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

type PortController {};

; type PortControllerRef (&mut PortController);

type U8Result (u8);

@Mod:port_inb extern ccc "port_inb" (u16) -> (u8);
@Mod:port_outb extern ccc "port_outb" (u16, u8) -> ();

; @Mod:heap_alloc extern ccc "heap_alloc" (u64) -> (*void);

; @PortController:inb input {
; 	; :port u16;
; } -> U8Result {
; 	; :0 mod:port_inb(input:port):0;
; 	:0 u8 123;
; };

@PortController:outb input {
	:port u16;
	:data u8;
} -> {
	mod:port_outb(input:port, input:data);
};

@Mod:port_controller_create input {} -> PortController {
	; raw := mod:heap_alloc(sizeof(PortController)):0;
	; :0 &mut PortController raw;
};
