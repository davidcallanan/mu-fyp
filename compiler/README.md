# Compiler

This directory contains the main entrypoint to the compiler.

The compilation process is separated into two phases:

- `frontend` (JavaScript): Parsing + initial analysis and transformations.
- `backend` (C++): Remaining analysis and transformations + codegen using LLVM.

While I intended to write the `backend` in JavaScript, the primary interface for LLVM is `C++`-based, so I'm writing this phase in `C++` for simplicity.

## Setup

- Install `pnpm ^9.15.1`.
