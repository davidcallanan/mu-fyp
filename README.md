# Essence C (Maynooth University - Final Year Project)

This respository acts as the definitive record of work for my final year project. I will try to keep as much of my work as possible within this repo - including code, writeups, notes, random thoughts.

The goal of this project is to develop a custom programming language known as "Essence C".

The language should include bespoke mechanism(s) to facilitate a flexible module dispatch mechanism - primarily achieved by fusing the notions of classes and modules that are traditionally separate in existing programming languages.

The end goal of this is to enhance the compiler's potential to perform link-time optimization on the resulting artifacts, resulting in small performance optimizations relative to other low-level languages (if things work out smoothly).

Additional research (if time permits) will be to study how this facilitates the continuum of both static and dynamic modules in kernel development. This may (if time permits) involve writing some simple kernel code in my programming language and analyzing performance gains relative to C code. The goal will be to get the project working before focusing on these aspects though.

One core aspect of the project will be attempting to maintain interconnectivity with existing low-level infrastructure, as the language must be suitable for kernel development. It is also pretty much guaranteed that LLVM IR will be the target (and I want to look at Clang's link-time optimizations when it uses LLVM IR). If I am using LLVM IR, the easiest approach will be to support interconnectivity between my language and LLVM IR, rather than offering direct integration with any other programming languages. LLVM IR also supports inline assembly really effectively (and this should be adequate for kernel development although I would have to look into this). My compiler might have to do some analysis of outputted LLVM IR files and even do some minor mutations to them to facilitate my performance optimizations (I think this is likely given that we may link foreign LLVM IR code and we may need to touch this code to get the best out of our optimizations across module boundaries, which is what I would want to consider this an absolute success).

Of course, this final year project is really just initial research and development of something that could take years of work. So I must manage my expectations and remember that I am not creating a fully-functional programming language that is ready for production use, or anything close to it.

## Project Structure

- `compiler` houses the actual compilation process which takes in language code and spits out a final executable.
- `extension` houses the VS Code extension that offers syntax highlighting to the programmer.
- `XXXX_syntax_ideas` contains some early syntax ideas ideas which have been significantly deviated from.

## Git Submodules

When cloning this repo, make sure git "submodules" are enabled and are recursively cloned.
