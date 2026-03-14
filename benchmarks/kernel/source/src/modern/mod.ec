; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

; Mod is the built-in module type.
; We extend it here with a port_manager field so the rest of the benchmark
; code can reach hardware I/O through mod:port_manager.

@Mod:port_manager &mut PortManager;

create extern ccc "run_benchmarks" () -> {
	; mod:port_manager = mod:port_manager_create {};

	; mod:print_set_color(mod:print_color_white, mod:print_color_black);
	mod:print_str("\n\n=== Port I/O Benchmark ===\n");
	; mod:print_set_color(mod:print_color_light_gray, mod:print_color_black);
	mod:print_str("\nBenchmarks: PIT, RTC, IO Wait, VGA Cursor");
	mod:print_str("\nDuration per benchmark: ");
	; mod:print_uint64_dec(mod:benchmark_duration_seconds);
	mod:print_str("s");

	; mod:benchmark_pit {};
	; mod:benchmark_rtc {};
	; mod:benchmark_io_wait {};
	; mod:benchmark_vga_cursor {};

	; mod:print_set_color(mod:print_color_white, mod:print_color_black);
	; mod:print_str("\n\n=== Benchmarks Complete ===");
}
