; 2026-03-12: This file was AI-generated (Claude Sonnet 4.6) as part of LLM Comprehension Testing (see Evaluation section in thesis)

@Mod:benchmark_wait_for_boundary input {} -> {
	mut orig := mod:port_manager:rtc_seconds {}:0;
	mut next := orig;

	for {
		next = mod:port_manager:rtc_seconds {}:0;
		if (next != orig) {
			break;
		}
	}
};

@Mod:benchmark_head input {
	:name *u8;
} -> {
	mod:print_set_color(mod:print_color_yellow, mod:print_color_black);
	mod:print_str("\n\n>> ");
	mod:print_str(input:name);
};

@Mod:benchmark_dump input {
	:name *u8;
	:count u64;
} -> {
	mod:print_set_color(mod:print_color_cyan, mod:print_color_black);
	mod:print_str("\n");
	mod:print_str(input:name);
	mod:print_str(": ");
	mod:print_uint64_dec(input:count);
	mod:print_str(" iterations in ");
	mod:print_uint64_dec(mod:benchmark_duration_seconds);
	mod:print_str(" seconds");
};

@Mod:benchmark_pit input {} -> {
	mod:benchmark_head { :name "\n\n>> PIT" };
	mod:benchmark_wait_for_boundary {};

	start := mod:port_manager:rtc_seconds {}:0;
	mut count := u64 0;

	for {
		mod:port_manager:pit_read {};
		count = count + u64 1;

		check := count % mod:benchmark_clock_interval;
		if (check == u64 0) {
			seconds := mod:port_manager:rtc_seconds {}:0;
			diff := seconds - start;
			elapsed := u8 diff;
			duration := u8 mod:benchmark_duration_seconds;
			if (elapsed >= duration) {
				break;
			}
		}
	}

	mod:benchmark_dump { :name "PIT", :count count };
};

@Mod:benchmark_rtc input {} -> {
	mod:benchmark_head { :name "\n\n>> RTC" };
	mod:benchmark_wait_for_boundary {};

	start := mod:port_manager:rtc_seconds {}:0;
	mut count := u64 0;

	for {
		mod:port_manager:rtc_seconds {};
		count = count + u64 1;

		check := count % mod:benchmark_clock_interval;
		if (check == u64 0) {
			seconds := mod:port_manager:rtc_seconds {}:0;
			diff := seconds - start;
			elapsed := u8 diff;
			duration := u8 mod:benchmark_duration_seconds;
			if (elapsed >= duration) {
				break;
			}
		}
	}

	mod:benchmark_dump { :name "RTC", :count count };
};

@Mod:benchmark_io_wait input {} -> {
	mod:benchmark_head { :name "\n\n>> IO Wait" };
	mod:benchmark_wait_for_boundary {};

	start := mod:port_manager:rtc_seconds {}:0;
	mut count := u64 0;

	for {
		mod:port_manager:io_wait {};
		count = count + u64 1;

		check := count % mod:benchmark_clock_interval;
		if (check == u64 0) {
			seconds := mod:port_manager:rtc_seconds {}:0;
			diff := seconds - start;
			elapsed := u8 diff;
			duration := u8 mod:benchmark_duration_seconds;
			if (elapsed >= duration) {
				break;
			}
		}	
	}

	mod:benchmark_dump { :name "IO Wait", :count count };
};
		
@Mod:benchmark_vga_cursor input {} -> {
	mod:benchmark_head { :name "\n\n>> VGA Cursor" };
	mod:benchmark_wait_for_boundary {};

	start := mod:port_manager:rtc_seconds {}:0;
	mut count := u64 0;

	for {
		mod:port_manager:vga_cursor_update(u16 0);
		count = count + u64 1;

		check := count % mod:benchmark_clock_interval;
		if (check == u64 0) {
			seconds := mod:port_manager:rtc_seconds {}:0;
			diff := seconds - start;
			elapsed := u8 diff;
			duration := u8 mod:benchmark_duration_seconds;
			if (elapsed >= duration) {
				break;
			}
		}
	}

	mod:benchmark_dump { :name "VGA Cursor", :count count };
};
