# Compiler

This directory contains the main entrypoint to the compiler.

The compilation process is separated into two phases:

- `frontend` (JavaScript): Parsing + initial analysis and transformations.
- `backend` (C++): Remaining analysis and transformations + codegen using LLVM.

While I intended to write the `backend` in JavaScript, the primary interface for LLVM is `C++`-based, so I'm writing this phase in `C++` for simplicity.

## Setup

- If on Windows, use WSL, this is mandatory. Otherwise use Linux.
- Install `nvm` ([Node Version Manager](https://github.com/nvm-sh)).
- Install the latest version of Node.js (^25.6.1) via `nvm install node` (or install Node.js using an alternative method).
- Install the latest version of `pnpm` (^10.24.0) via `npm install -g pnpm`.

## Run

- `node ec.js compile <src> <dest>` where `<dest>` configurese where the output artifacts go.
