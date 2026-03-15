MODE ?= MODE_A

# MODE_A: Build using C version of benchmarks.
# MODE_B: Build using EC version of benchmarks.

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
LLC := llc-16

kernel_source_files := $(shell find src/impl/kernel -name *.c)
ifeq ($(MODE),MODE_B)
kernel_source_files := $(filter-out src/impl/kernel/benchmark.c, $(kernel_source_files))
endif
kernel_object_files := $(patsubst src/impl/kernel/%.c, build/kernel/%.o, $(kernel_source_files))

x86_64_c_source_files := $(shell find src/impl/x86_64 -name *.c)
ifeq ($(MODE),MODE_B)
x86_64_c_source_files := $(filter-out src/impl/x86_64/port_controller.c src/impl/x86_64/port_manager.c src/impl/x86_64/mod.c, $(x86_64_c_source_files))
endif
x86_64_c_object_files := $(patsubst src/impl/x86_64/%.c, build/x86_64/%.o, $(x86_64_c_source_files))

x86_64_asm_source_files := $(shell find src/impl/x86_64 -name *.asm)
ifeq ($(MODE),MODE_B)
x86_64_asm_source_files := $(filter-out src/impl/x86_64/port_.asm, $(x86_64_asm_source_files))
endif
x86_64_asm_object_files := $(patsubst src/impl/x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

llvm_source_files := $(shell find src/modern -name *.ll)
ifeq ($(MODE),MODE_A)
llvm_source_files :=
endif
llvm_object_files := $(patsubst src/modern/%.ll, build/modern/%.o, $(llvm_source_files))

x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)

build/kernel/%.o: src/impl/kernel/%.c
	mkdir -p $(dir $@)
	$(CC) -c -I src/intf -ffreestanding $(patsubst build/kernel/%.o, src/impl/kernel/%.c, $@) -o $@

build/x86_64/%.o: src/impl/x86_64/%.c
	mkdir -p $(dir $@)
	$(CC) -c -I src/intf -ffreestanding $(patsubst build/x86_64/%.o, src/impl/x86_64/%.c, $@) -o $@

build/x86_64/%.o: src/impl/x86_64/%.asm
	mkdir -p $(dir $@)
	nasm -f elf64 $(patsubst build/x86_64/%.o, src/impl/x86_64/%.asm, $@) -o $@

build/modern/%.o: src/modern/%.ll
	mkdir -p $(dir $@)
	$(LLC) -march=x86-64 -relocation-model=static -filetype=obj $< -o $@

.PHONY: build-x86_64
build-x86_64: $(kernel_object_files) $(x86_64_object_files) $(llvm_object_files)
	mkdir -p dist/x86_64
	$(LD) -n -o dist/x86_64/kernel.bin -T targets/x86_64/linker.ld $(kernel_object_files) $(x86_64_object_files) $(llvm_object_files)
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel.iso targets/x86_64/iso
	
.PHONY: build-a
build-a:
	make build-x86_64 MODE=MODE_A
	
.PHONY: build-b
build-b:
	make build-x86_64 MODE=MODE_B

.PHONY: clean
clean:
	rm -rf build dist
