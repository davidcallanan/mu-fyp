# Essence C Language Reference

Essence C (`.ec`) is a compiled, statically-typed language that compiles to native code via LLVM. This document is intended as a concise reference for porting C programs into Essence C.

---

## Comments

Comments begin with `;` and run to end of line. There are no block comments.

```ec
; this is a comment
x := 10; this is also a comment
```

---

## Primitive Types

| Type | Description |
|------|-------------|
| `u8`, `u16`, `u32`, `u64` | Unsigned integers |
| `i8`, `i16`, `i32`, `i64` | Signed integers |
| `usize` | Pointer-sized unsigned integer |
| `f32`, `f64` | Floating-point |
| `bool` | Boolean |
| `*T` | Pointer to `T` |
| `**T` | Pointer to pointer to `T` |

---

## Type Aliases

Use `type` at the top level to define an alias for an existing type.

```ec
type MyInt u64;
type String *u8;
type CharPtrPtr **u8;
type MyStruct {};
```

An empty `{}` defines an opaque map type (struct-like).

---

## Variables

Declare with `:=` (immutable by default). Use `mut` for mutable variables.

```ec
x := 42;
name := "hello";
ptr := *u8 "hello";
mut counter := u64 0;
```

Assignment to a mutable variable uses `=`:

```ec
counter = counter + 1;
```

Typed coercion syntax — `TypeName value` — coerces a literal or variable to a specific type:

```ec
n := u64 10;
f := f32 3.14;
s := String "world";
flag := bool :true;
```

---

## Literals

- **Integer**: `42`, `0`, `1000`
- **Float**: `3.14`, `0.5`
- **String**: `"hello\n"` — supports `\n` and `\\` escape sequences
- **Symbol (enum variant)**: `:true`, `:false`, `:ok`, `:error`

---

## Symbols and Enums

Symbols are prefixed with `:`. An enum type is declared inline with `enum { }`:

```ec
flag := bool :true;

status := enum {
	:ok;
	:error;
} :ok;
```

Single-line form:

```ec
status := enum { :ok, :error } :ok;
```

---

## Type-Value Unification

In Essence C, types and values share the same syntax — everything is a *constraint*. A literal value is simply a singleton constraint (exactly one possible value). The compiler merges multiple constraints to determine the actual runtime representation.

This means a variable can be assigned to:
- A pure type (a non-singleton constraint, e.g. just `u64`)
- A pure value (a singleton constraint, e.g. just `42`)
- Both together (a type coercion, e.g. `u64 42`)

```ec
a := u64;       ; type only — no runtime value yet (used in struct field declarations)
b := 42;        ; value only — compiler infers minimal integer type
c := u64 42;    ; type + value — explicit coercion
```

The compiler decides the encoding (bit-width, representation) by merging all constraints it can observe.

---

## Map-Enum Equivalence

Calling a map with a symbol (`:foo`) and then calling the result with another argument is semantically identical to calling the map with a fully populated enum variant (`:foo(arg)`). This is the *map-enum equivalence* rule, which unifies structs, functions, and enums into one coherent concept.

In practice this means you rarely need to think about enums vs structs — both are just maps accessed via symbols.

---

## Arithmetic & Comparison

Standard infix operators for binary expressions:

```ec
result := a + b;
result := a - b;
result := a * b;
result := a / b;
```

Multi-operand **additive crystal** (all operands at same precedence, each on its own line):

```ec
total :=
	+ 50
	+ 70
	- 30
;
```

Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`.

```ec
is_ten := x == 10;
is_positive := x > 0;
```

---

## Logical Operators

Logical AND and OR use prefix-style multi-operand crystal syntax:

```ec
both :=
	&& condition_a
	&& condition_b
;

either :=
	|| condition_a
	|| condition_b
;
```

Do not mix `&&` and `||` in one sequence; nest them using the grouping syntax `(: expr )` (see below) or an intermediate variable:

```ec
; using (: ) grouping
result :=
	|| (: condition_a && condition_b )
	|| condition_c
;

; equivalently, using an intermediate variable
both :=
	&& condition_a
	&& condition_b
;

result :=
	|| both
	|| condition_c
;
```

---

## Grouping Expressions

Wrap any expression in `(: ` and `)` to form a grouped sub-expression. The opening delimiter is `(:` (optionally with whitespace before the inner expression); the closing delimiter is `)`. This is the only parenthesis-grouping syntax in the language — plain `( expr )` is **not** valid for grouping.

```ec
; force evaluation order in arithmetic
x := (: a + b ) * c;

; group a logical sub-expression inline
result :=
	|| (: && condition_a && condition_b )
	|| condition_c
;

; works anywhere an expression atom is valid
flag := (: bool :true );
```

The `:` in the delimiter is intentional: it is reserved for a future "leaf return" feature, so plain `(` remains available for existing tuple/call syntax without ambiguity.

---

## Crystal vs Pistol Syntax

All associative operator families (additive, multiplicative, logical AND, logical OR) support two equivalent syntactic forms:

- **Crystal** — a unary-style batch where each operand is prefixed by its operator. This extends naturally to any number of operands and reads top-to-bottom.
- **Pistol** — a conventional left-to-right binary infix form.

These forms are interchangeable; precedence is the same. The crystal form is preferred for multi-operand expressions.

```ec
; Crystal (multi-operand, leading operator per line)
total :=
	+ a
	+ b
	- c
;

; Pistol (binary infix)
total := a + b - c;

; Both are legal for multiplication too
product :=
	* x
	* y
;
```

Precedence order (highest to lowest): multiplicative → additive → logical AND → logical OR.

---

## Control Flow

### `if` / `else if` / `else`

```ec
if (x == 0) {
	log("zero");
} else if (x < 0) {
	log("negative");
} else {
	log("positive");
}
```

### `for` (infinite loop)

```ec
for {
	; ... body ...
	if (done) {
		break;
	}
}
```

Only `break` is available to exit a loop. There is no `continue`.

---

## Maps (Struct-like Objects)

A map literal is `{ :sym value; ... }`. Members are accessed with `:sym` chained after the variable.

```ec
person := {
	:name "Alice";
	:age u64 30;
};

log(person:name);
n := person:age;
```

Nested maps:

```ec
data := {
	:inner {
		:x u64 1;
	};
};

v := data:inner:x;
```

---

## Callables (Functions / Lambdas)

A callable is declared with `input { ... } -> { ... }`. `input` is optional as a label but required syntactically.

```ec
greet := input {
	:name *u8;
} -> {
	log(input:name);
};

greet {
	:name "Bob";
};
```

Callables can also be written inline inside a map:

```ec
obj := {
	input {
		:x u64;
	} -> {
		log_d(input:x);
	}
};
```

### Tuple-style calling (positional arguments)

Instead of named map arguments you can use parentheses with positional values. Positional parameters are accessed via `:0`, `:1`, etc.:

```ec
greet("Alice");          ; calls with :0 = "Alice"
add(10, 20);             ; calls with :0 = 10, :1 = 20
```

---

## `sizeof`

Returns the size in bytes of a value or type expression.

```ec
s := sizeof(x);
log_d(s);
```

---

## Address-of

`&expr` takes the address of an expression (returns a pointer):

```ec
ptr := &some_value;
```

---

## Top-level Type Extensions (`@`)

Attach named members (including callables and extern functions) to a named type using `@TypeName:member_name value`:

```ec
type Vec2 {};

@Vec2:x f32;
@Vec2:y f32;

@Vec2:length input {} -> {
	; method body; "this" refers to the Vec2 instance
	log(this:x);
};
```

Multi-level paths for nested member attachment:

```ec
@Vec2:math:dot input {
	:other Vec2;
} -> {
	; ...
};
```

`this` refers to the receiver object. `mod` refers to the current module.

---

## Module-level Members (`@Mod`)

Attach members to the current module using `@Mod:name value`:

```ec
@Mod:app_name "My App";

@Mod:run input {} -> {
	log(mod:app_name);
};
```

Call module members via `mod:name`:

```ec
mod:run {};
```

---

## Entry Point

Every module must have exactly one `create` block. This is the entry point. By default the compiled entry function is exported as `main`:

```ec
create() -> {
	; program starts here
}
```

### `extern ccc` on `create` — custom export symbol

Use `extern ccc "symbol_name"` on the `create` block to export the entry point under a custom C-ABI symbol name instead of `main`. This is useful when the module is intended to be called from C or another language (e.g. a kernel entry point, a library initialiser, or a custom linker script symbol):

```ec
create extern ccc "kernel_main" () -> {
	; exported as "kernel_main" via the C calling convention
}
```

Without `extern ccc` the function is still exported as `main` and callable from C, but the `extern ccc` form makes the symbol name explicit.

---

## Calling External C / LLVM IR Functions (C ABI)

Use `extern ccc "symbol_name"` inside an `@` extension to bind a C ABI function. `ccc` means the C Calling Convention.

### Syntax

```ec
@TypeName:method_name extern ccc "c_function_name" (param_types) -> (return_types);
```

Parameters and return types use tuple syntax. An empty tuple `()` means void.

### Examples

```ec
type Libc {};

; void printf(const char* fmt, ...)
; Note: variadic args not yet supported; bind single-call signatures
@Libc:printf extern ccc "printf" (*u8) -> ();

; int getpid(void)
@Libc:getpid extern ccc "getpid" () -> (i32);

; void* malloc(size_t n)
@Libc:malloc extern ccc "malloc" (u64) -> (*u8);

; void free(void* ptr)
@Libc:free extern ccc "free" (*u8) -> ();

; int puts(const char* s)
@Libc:puts extern ccc "puts" (*u8) -> (i32);
```

### Calling extern functions

Attach to `Mod` to call without instantiating a type object:

```ec
@Mod:printf extern ccc "printf" (*u8) -> ();
@Mod:getpid extern ccc "getpid" () -> (i32);

create() -> {
	mod:printf("Hello, World!\n");

	result := mod:getpid();
	log_d(result:0);     ; access first return value with :0
}
```

Named argument form (using `:0`, `:1` for positional):

```ec
mod:printf {
	:0 "Hello\n";
};
```

### `alwaysinline` call flag

Apply `alwaysinline` between the callable and its argument map to instruct the LLVM backend to emit the `alwaysinline` attribute on the generated call function, forcing the optimizer to inline the call across the call boundary. This is the key flag for the *static* implementation variant, enabling cross-module inlining via LTO.

```ec
mod:printf alwaysinline {
	:0 "Inlined!\n";
};
```

The `alwaysinline` flag can also be used with named-argument map syntax:

```ec
some_callable alwaysinline {
	:x u64 10;
};
```

---

## Accessing Return Values

Return values from extern (or any) calls are accessed positionally via `:0`, `:1`, etc. on the result map:

```ec
ret := mod:getpid();
pid := ret:0;
log_d(pid);
```

---

## Debugging Utilities

These are built-in and do not require extern declarations.

```ec
log("some string");        ; print a string
log();                     ; print empty line
log_d(value);              ; print a value as hex + ascii + decimal
log_dd(ptr, null-term);    ; dereference and print a null-terminated string
log_dd(ptr, 16);           ; dereference and print 16 bytes
```

---

## No Standard Library — Freestanding Design

Essence C has no standard library. It is designed for freestanding environments (e.g. OS kernels). The only built-in external dependency is `puts` (used internally by `log`/`log_d`). All other library functions must be declared as `extern ccc` bindings manually.

This means all C standard library functions (`printf`, `malloc`, `free`, `memcpy`, etc.) must each have an `@Mod:name extern ccc "symbol" (...) -> (...);` declaration before use.

---

## Singletonish Optimization and `mut`

By default, variables are immutable (`const`). When the compiler can prove that a map instance holds only compile-time-constant data (no mutable state), it applies the *singletonish* optimization: the map is collapsed to a zero-size struct and all its field accesses become compile-time constants, allowing the optimizer to eliminate runtime overhead entirely.

Declaring a variable with `mut` opts out of this optimization for that variable, signalling that the value may change at runtime:

```ec
mut counter := u64 0;   ; runtime-mutable; singletonish NOT applicable
counter = counter + 1;
```

For the *static dispatch* variant of a module, omitting `mut` on instance variables (combined with `alwaysinline` calls) is what allows the compiler to fully inline and eliminate indirection.

---

## Full Example: Porting a C Program

**Original C:**

```c
#include <stdio.h>
#include <stdlib.h>

int add(int a, int b) {
    return a + b;
}

int main(void) {
    int x = add(3, 4);
    printf("result: %d\n", x);
    return 0;
}
```

**Essence C equivalent:**

```ec
type Libc {};

@Libc:printf extern ccc "printf" (*u8) -> ();

@Mod:printf extern ccc "printf" (*u8) -> ();

add := input {
	:a i32;
	:b i32;
} -> {
	; return value is the last expression in the block
	input:a + input:b
};

create() -> {
	result := add {
		:a i32 3;
		:b i32 4;
	};

	mod:printf("result: %d\n");   ; NOTE: formatted printing requires the real printf with variadic args
	                               ; for now, use log_d for numeric output
	log_d(result);
}
```

---

## Key Patterns When Porting C

| C concept | Essence C equivalent |
|-----------|----------------------|
| `int x = 5;` | `x := i32 5;` |
| `int* p = &x;` | `p := &x;` |
| `typedef int MyInt;` | `type MyInt i32;` |
| `struct Foo { int x; }` | `type Foo {};` then `@Foo:x i32;` |
| `void fn(int a)` | `@Mod:fn input { :a i32; } -> { ... };` |
| `extern int puts(const char*)` | `@Mod:puts extern ccc "puts" (*u8) -> (i32);` |
| `puts("hi")` | `mod:puts("hi");` |
| custom entry symbol (e.g. `_start`) | `create extern ccc "_start" () -> { ... }` |
| `for (;;) { ... break; }` | `for { ... break; }` |
| `if (c) { } else { }` | `if (c) { } else { }` |
| `a && b` | `&& a && b` (crystal) or `a && b` (pistol) |
| `a \|\| b` | `\|\| a \|\| b` (crystal) or `a \|\| b` (pistol) |
| `(a && b) \|\| c` | `\|\| (: && a && b ) \|\| c` or use intermediate variable |
| `sizeof(x)` | `sizeof(x)` |
| `__attribute__((always_inline))` | `alwaysinline` call flag |
| `const int x = 5;` | `x := i32 5;` (immutable by default) |
| `volatile int x = 5;` | `mut x := i32 5;` |

---

## Module Structure Skeleton

```ec
; 1. Type aliases
type MyType {};

; 2. Type extensions (fields and methods)
; Inside a method body:
;   "input" refers to the call argument map
;   "this"  refers to the receiver instance (the MyType object)
;   "mod"   refers to the current module (Mod)
@MyType:field_name u64;
@MyType:do_thing input { :x u64; } -> {
	log_d(input:x);
	log(this:field_name);   ; access receiver field
};

; 3. Module-level externs
@Mod:puts extern ccc "puts" (*u8) -> (i32);

; 4. Module-level helpers
@Mod:helper input { :n u64; } -> {
	log_d(input:n);
};

; 5. Entry point
create() -> {
	inst := MyType;
	inst:do_thing { :x u64 42; };
	mod:puts("done\n");
}
```
