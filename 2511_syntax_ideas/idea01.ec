sources {
	; probably will scrap a regular sources block because we already have a layer of naming indirection when "importing" should that not be sufficient?
	; each import then involves a concious choice of using either "/" (i own the module!) or "//" (i am using something external). it doesn't require much effort to change between them if you change your mind.
}

forwarding {
	reset //;
	; if reset // appears here, it wouldn't be forwarded but would still be accessible in forwarding blocks for individual imports to override if so wished.
	; the default strategy is not to reset, so that forwarding is automatic.
	; LHS can only be // here, but RHS can be / or //.
	; resets must be the very first thing if wanted, otherwise no reset.
	; every module implicitely resets /.
}

; the key is that a module always owns its types when it comes to the type system (irrespective of whether it owns the codebase when it comes to importing).
; thus it seems rather convenient to be able to export a namespace.
public types foo import /intf/foo;

types foo import /intf/foo;

types foo_bar import /intf/bar forwarding {
	reset //;
	//intf/foo /intf/foo;
	//banana/apple //banana/apple;
	; like other forwarding blocks, LHS must be //, but RHS can be / or //.
}

type Foo foo_bar::FooBarf;

public type Bar foo_bar::Bar;

type Bar enum {
	:my_entry_name;
	:my_other_entry_name map (
		:vfs Foo;
		:bar i32;
	);
}

type Foo map {
	:vfs Foo; syntactic sugar for :vfs map (: Foo);
	:blah *Bar;
	:point map (i32, i32, i32);
	:distance map map (:x i32, :y i32) i32; i don't want to use this syntax
	:distance map (:x i32, :y i32) -> i32; i think this looks better
	; the idea is that "->" automatically means a map that takes in non-sym input
	; of course then in the following line we make the current map itself callable with non-sym.
	; internally it is taught of as a sort of ":call" symbol, because i like making anything callable.
	map (:x i32, :y i32) -> f64; i think this is good stuff
	: Foo; this is what the type is when not treated as a map. the "leaf value" as i have called it for many years.
}

type DeliciousInteger i33;

; now in source modules, we need to think how we take in dependencies and all that and instantiate a module
; before i think i was thinking that the source module specifies things like how it wants to grab these dependencies (static vs dynamic), but i think the module user should be able to control that.
; really dependencies is just a map?

dependencies map {
	:foo Foo; we want an instance of foo
	:bar_factory BarFactory;
}

options map {}

; however i don't want to create special language constructs for this. i wonder is it possible to make module instantiating a regular function. well, it is indeed a comptime function, or is it.
; no it's not, it can be runtime, so we can pass in dependencies.
; but at some stage we can specify that a variable is "static" or something like this, and the compiler has to trace that and remember it.
; the tracing might be difficult.

; i guess first, we should look at how do we import a source module compared to intf, and probably similar.

types foo import /impl/foo;

; i guess `foo` now has some types in it but then there would also be a way to instantiate it.
; there could be a cool thing i mention in my report "compile-time tracing of the static vs dynamic nature through runtime code".

; maybe we just take stuff "in".

in map {
	:dependencies map {
		:foo Foo; we want an instance of foo
		:bar_factory BarFactory;
	}
	
	:options map {
		:lru_size i32;
	}
};

; one can repeat "in map" blocks and they get merged.
; we might go ahead and force "in" map to be a non-tuple map to enable this straightforward merge behaviour.

; any instance of a module keeps track of whether it is a runtime or comptime instance.
; some simple rules can then be applied:
; when we create an instance of a module, and pass it a dependency, if the dependency is clearly comptime (say at the global scope), the instance remembers this.
; if runtime then also remembers this.
; but the key is that comptime can be reduced to runtime (if any chance of runtime) but runtime can not ever go back up to comptime.
; we can enforce comp-time perhaps through a little keyword (and simply the compiler would choose to bail out if it can't prove it).
; by default when you create an instance of a module, it is made as comptime if the instance itself is comptime (and maybe instead of comptime we are thinking of the word "static").

; im trying to think it through. i guess the root "entry" module we compile ought to be comptime.
; if we access something straight from "input" then this comptime is preserved.
; but let's say we call some function or something to obtain the instance, then comptime is gone.
; wait, but what about when we genuinely instantiate a module instance.... ahhh

; i think it comes back to, do I own the instance or not? (similar to do i own the code?)

; let's suppose we instantiate a module instance...
; i guess a namespace can have attached to it an implementation, allowing instantiation.
; it's like the idea of leaf functions, but here, we are thinking to ourselves that there are no leaf functions anymore, just the one "instantiation" function. further non-leaf instantiation functions can always be created if so desired.

my_module := instantiate foo(
	:dependencies {
		; blah
	}
	:options {
		; blah
	}
);

; notice that this instantiation is done at the global scope, thus it can inherit the comptime of the current module instance.
; while properties in maps could also keep track of comptime vs runtime, i feel this might be too complex for a fyp that is due in March.
; so let's just simplify for now and so VARIABLES at module scope keep track of comptime vs runtime.
; variables are easier to deal with.. don't got the complexities of a map with types and all that.

; note though: the "input" is a map, so we do have to keep track through maps ahhhhh

; right how via gonna track via maps? remember we have a few possibilities, we have static, we have thread-local storage or global variable, and we have dynamic.
; however, we can assume tls and gv are static, as a static implementation can wrap the dynamic nature.
; we can use LTO (link time optimization) to easily do this (which would be great to talk about in my report).

; in my report: "technically C does not have to even compile to object files, it could compile each file to an executable file and then use an overkill runtime linkage mechanism, the point is that there is room for alteration in this".

; so all "sym" map instances (don't care about non-sym variants) must keep track of their comptime vs runtime status, collapsing to the worst-case scenario (kind of like how a typesystem collapses its possible values).

; by default, dispatch is "static", but there would be perhaps three options for this demo.

my_module := instantiate foo(
	
) dispatch {
	static force
};

my_module := instantiate foo(
	
) dispatch {
	global
};

my_module := instantiate foo(
	
) dispatch {
	dynamic
};

; force will ensure that the compiler bails out if not possible, but by default, it is just a recommendation.

; when one instantiates a module in another scope, then dispatch is ignored and it is just treated as dynamic, like instantiating a class.

; however, what if we instantiate two modules that both implement that interface and might pass through?
; i think when we say dynamic, we do in fact mean double dispatch, unfortunately. there is an element of saying the implementation is fixed, but its dependencies etc. is dynamic, so maybe we can consider dyanamic-single vs dynamic-double.
; even some of these things can be put through as considerations in my report, whether i implement or not (because implementing dynamic gonna be hell of a lot easier).

; i think i can make effort to propogate dyanmic-single vs dynamic-double,,, we'll seee..



; im thinking when importing a source module it will instaed be

mod my_module import /impl/my_module;

; we distinguish between "mod" and "ns" because "mod" has instantiation function.




