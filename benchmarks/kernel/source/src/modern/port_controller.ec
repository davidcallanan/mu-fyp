; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

type PortController {};

type U8Result (u8);

@Mod:port_inb extern ccc "port_inb" (u16) -> (u8);
@Mod:port_outb extern ccc "port_outb" (u16, u8) -> ();

@Mod:heap_alloc extern ccc "heap_alloc" (usize) -> (*u8);

@PortController:inb input {
	:port u16;
} -> U8Result {
	:0 mod:port_inb(input:port):0;
};

@PortController:outb input {
	:port u16;
	:data u8;
} -> {
	mod:port_outb(input:port, input:data);
};

@Mod:port_controller_create input {} -> PortController {
};
