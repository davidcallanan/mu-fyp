forwarding {
	reset //;
}

forwarding {
	reset //;
	reset //foo/bar/baz;
	//banana/slop /wanana/bop;
}

type foo::Bar f32;
type foo::bar::Baz Banana;
type foo::bar::Bink foo::Bar;
type foo::Coconut map;

types egg::plant foo::bar;

types banana import /foo/bar;

types orange import /foo/bar forwarding {
	reset //;
	reset //foo/bar/baz;
	//banana/slop /wanana/bop;
}

mod eggplant import //egg/plant;

mod eggplant import //egg/plant forwarding {
	//pear /pear;
}

; todo why is eggplant2 not working?
