# Kernel Benchmark

This directory houses the kernel benchmark.

Six years ago, I wrote a [prototype 64-bit operating system kernel](https://github.com) using C and assembly.

This code has been forked into the "source" directory.

The code has been adapted to include the following four benchmarks:

- **PIT**: This benchmark accesses the PIT counter as frequently as it can. This is a counter exposed by the x86 hardware that simply decreases gradually over time.
- **RTC**: This benchmark accesses the Real-Time Clock as frequently as it can, exposed once again by the x86 hardware. The irony is that this Real-Time Clock is furthermore used in *all* benchmarks to actually compute the performance.
- **IO Wait**: This benchmarks triggers the "IO Wait" hardware request as frequently as it can, which is essentially a legacy no-op instruction that gives the CPU a moment to breathe (in simple terms).
- **VGA Cursor**: This benchmark updates the VGA text cursor position as frequently as it can. This cursor position controls where text would be printed on the screen when using the basic VGA text interface exposed by the hardware.

All the above data is gathered via the x86 port I/O interface, so the logic isn't too complicated to implement. It tests real-world communication (albeit simple) with hardware, which can only be done in an operating system kernel. The port communication must be performed using assembly instructions which is a good use-case of testing crossing the boundary between compilation units, revealing how using a combination of LLVM IR link-time optimization and my custom programming language has genuine potential.

A portion of the code has two versions, so that we can benchmark the difference in performance:

- The "C" + "assembly" version. We benchmark how many operations the C language can perform per second using no bespoke optimizations.
- The "Essence C" + "LLVM IR assembly" version. We benchmark how many operatoins the Essence C language can perform per second both without bespoke optimizations and using the custom inlining optimization.

Could the C code be optimized? While in this case it could, we are intentionally showing a codebase where this optimization cannot be enabled. This is because C traditionally has this notion of separate compilation units as a way of organizing code. This can be bypassed in two ways, by using LLVM's layer as object files, which can improve performance significantly. However the core language feature of Essence C is the module system that is designed such that the user is not forced to architect their software in a particular way. In Essence C, modules are just an organizational construct on top of maps, and it is the maps themselves that enjoy the optimization directly. In Essence C, almost everything is a map. Furthermore, optimizing the C code also requires using LLVM IR for assembly which is messy. This language has first class integration with LLVM IR, alongside callsite "alwaysinline" flags, that would theoretically be removed down the road once the static analysis of my language improved.

The core in this language is that we can instantiate multiple instances of a map, we can suddenly decide to have it runtime based. We can simulate this by manually adding or removing the "alwaysinline" flag as the static analysis is not good enough yet. Some final work needs to be done on the "singletonish" optimization which tracks instances of objects in the codebase and determines if there is only one possible value at runtime - then it can be optimized - and the ability to implement closures that genuinely store information, wait, we can already store information in maps, maybe i can just about get this implemented before the FYP is due, we'll see.

The version in C intentionally shows how we instantiate a module instance, then instantiate an instance of a port system inside that. This allows for using closures at any point in the chain, that is, adding contextual data, and suddenly this breaks the optimization potential.
