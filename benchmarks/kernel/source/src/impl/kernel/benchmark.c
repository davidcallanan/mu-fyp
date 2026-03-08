#include "kernel/benchmark.h"

#include "x86_64/mod.h"
#include "print.h"

#define BENCHMARK_DURATION_SECONDS 5
#define BENCHMARK_CLOCK_INTERVAL 100

static void benchmark_wait_for_boundary(struct Mod* mod) {
	uint8_t orig = mod->port_manager->rtc_seconds(mod->port_manager);
	uint8_t next = orig;

	while (next == orig) {
		next = mod->port_manager->rtc_seconds(mod->port_manager);
	}
}

static void benchmark_head(char* name) {
	print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
	print_str("\n\n>> ");
	print_str(name);
}

static void benchmark_dump(char* name, uint32_t count) {
	print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
	print_str("\n");
	print_str(name);
	print_str(": ");
	print_uint64_dec(count);
	print_str(" iterations in ");
	print_uint64_dec(BENCHMARK_DURATION_SECONDS);
	print_str(" seconds");
}

static void benchmark_pit(struct Mod* mod) {
	benchmark_head("PIT");
	benchmark_wait_for_boundary(mod);
	
	uint8_t start = mod->port_manager->rtc_seconds(mod->port_manager);
	uint32_t count = 0;

	while (1) {
		mod->port_manager->pit_read(mod->port_manager);
		count++;

		if (count % BENCHMARK_CLOCK_INTERVAL == 0) {
			uint8_t seconds = mod->port_manager->rtc_seconds(mod->port_manager);
			
			if ((uint8_t)(seconds - start) >= BENCHMARK_DURATION_SECONDS) {
				break;
			}
		}
	}

	benchmark_dump("PIT", count);
}

static void benchmark_rtc(struct Mod* mod) {
	benchmark_head("RTC");
	benchmark_wait_for_boundary(mod);
	
	uint8_t start = mod->port_manager->rtc_seconds(mod->port_manager);
	uint32_t count = 0;

	while (1) {
		mod->port_manager->rtc_seconds(mod->port_manager);
		count++;

		if (count % BENCHMARK_CLOCK_INTERVAL == 0) {
			uint8_t seconds = mod->port_manager->rtc_seconds(mod->port_manager);
			
			if ((uint8_t)(seconds - start) >= BENCHMARK_DURATION_SECONDS) {
				break;
			}
		}
	}

	benchmark_dump("RTC", count);
}

static void benchmark_io_wait(struct Mod* mod) {
	benchmark_head("IO Wait");
	benchmark_wait_for_boundary(mod);
	
	uint8_t start = mod->port_manager->rtc_seconds(mod->port_manager);
	uint32_t count = 0;

	while (1) {
		mod->port_manager->io_wait(mod->port_manager);
		count++;

		if (count % BENCHMARK_CLOCK_INTERVAL == 0) {
			uint8_t seconds = mod->port_manager->rtc_seconds(mod->port_manager);
			
			if ((uint8_t)(seconds - start) >= BENCHMARK_DURATION_SECONDS) {
				break;
			}
		}
	}

	benchmark_dump("IO Wait", count);
}

static void benchmark_vga_cursor(struct Mod* mod) {
	benchmark_head("VGA Cursor");
	benchmark_wait_for_boundary(mod);
	
	uint8_t start = mod->port_manager->rtc_seconds(mod->port_manager);
	uint32_t count = 0;

	while (1) {
		mod->port_manager->vga_cursor_update(mod->port_manager, 0);
		count++;

		if (count % BENCHMARK_CLOCK_INTERVAL == 0) {
			uint8_t seconds = mod->port_manager->rtc_seconds(mod->port_manager);
			if ((uint8_t)(seconds - start) >= BENCHMARK_DURATION_SECONDS) {
				break;
			}
		}
	}

	benchmark_dump("VGA Cursor", count);
}

void run_benchmarks() {
	struct Mod* mod = mod__create();

	print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
	print_str("\n\n=== Port I/O Benchmark ===\n");
	print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
	print_str("\nBenchmarks: PIT, RTC, IO Wait, VGA Cursor");
	print_str("\nDuration per benchmark: ");
	print_uint64_dec(BENCHMARK_DURATION_SECONDS);
	print_str("s");

	benchmark_pit(mod);
	benchmark_rtc(mod);
	benchmark_io_wait(mod);
	benchmark_vga_cursor(mod);

	print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
	print_str("\n\n=== Benchmarks Complete ===");
}
