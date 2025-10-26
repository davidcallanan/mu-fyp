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

type Foo map {
	:vfs map (Foo);
	:blah map (*Bar);
	:wah Foo; syntactic sugar for :wah map (Foo)
}

type Bar enum {
	:my_entry_name;
	:my_other_entry_name (
		
	);
}

type DeliciousInteger i33;
