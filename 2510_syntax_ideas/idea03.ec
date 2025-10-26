sources {
	/intf/foo //intf/foo;
	/intf/bar //intf/bar;
	; if reset /// appears here, it wouldn't be accessible in forwarding blocks
}

forwarded sources {
	reset ///;
	; if reset /// appears here, it wouldn't be forwarded but would still be accessible in forwarding blocks
}

public namespace foo = import /intf/foo;

namespace bar = import /intf/bar {
	reset //;
	//intf/foo /intf/foo
	//banana/apple //banana/apple
	//banana/grape ///banana/grape
}

type Foo = foo::Foo;

public type Bar = bar::Bar;

; should anything in triple be automatically available in double?
; maybe the other way around? if we access triple, it will fallback to double?
; no it doesn't do justice.
