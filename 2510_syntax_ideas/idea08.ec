sources {
	/intf/foo //intf/foo;
	/intf/bar //intf/bar;
	/intf/baz //intf/com.dcallanan/baz {
		; allows controlling with higher priority what is populated when this is later used
		; for example if this is later passed down to other modules,
		; it will base off the "//" at this scope,
		; not what "//" later happens to be.
		/blah/wah //blah/wah;
		; / always means the "module root" even though we could be touching a directory that does not reset the module root, recall the difference
		; between the raw filesystem and the module hierarchy.
		; thus / is the current module root irrespective of the directory or file mentioned.
		; and // is the current "outer root" that this module is operating under.
	}
	; if reset // appears here, it wouldn't be accessible in forwarding blocks
}

forwarded sources {
	reset //;
	; if reset // appears here, it wouldn't be forwarded but would still be accessible in forwarding blocks
}

public namespace foo import /intf/foo;

namespace foo_bar import /intf/bar {
	reset //;
	//intf/foo /intf/foo;
	//banana/apple //banana/apple;
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
	:distance map map (:x i32, :y i32) i32;
	:distance map (:x i32, :y i32) -> i32; i think this looks better
	map map (:x i32, :y i32) f64;
	map (:x i32, :y i32) -> f64; i think this is good stuff
	: Foo;
}

type DeliciousInteger i33;
