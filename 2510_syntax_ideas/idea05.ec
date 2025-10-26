sources {
	/intf/foo //intf/foo;
	/intf/bar //intf/bar;
	/intf/baz //intf/com.dcallanan/baz;
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
	:distance(:x i32, :y i32) i32;
	(:x i32, :y i32) f64;
	: Foo;
}

type DeliciousInteger i33;
