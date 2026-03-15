; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

target triple = "x86_64-unknown-elf"

; uint8_t port_inb(uint16_t port)
define i8 @port_inb(i16 %port) nounwind {
	%result = call i8 asm sideeffect "inb $1, $0", "={al},{dx}"(i16 %port)
	ret i8 %result
}

; void port_outb(uint16_t port, uint8_t data)
define void @port_outb(i16 %port, i8 %data) nounwind {
	call void asm sideeffect "outb $0, $1", "{al},{dx}"(i8 %data, i16 %port)
	ret void
}

declare void @print_str(i8* %string)
declare void @print_char(i8 %character)

; int puts(const char *s)
define i32 @puts(i8* %s) nounwind {
	call void @print_str(i8* %s)
	call void @print_char(i8 10)
	ret i32 0
}
