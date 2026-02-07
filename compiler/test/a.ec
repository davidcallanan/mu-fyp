forwarding {
	reset //;
}

forwarding {
	reset //;
	reset //foo/bar/baz;
	//banana/slop /foo/bop;
}

type foo::Bar f32;

; types foo::bar foo::bar;
type foo::bar::Baz Banana;
type foo::bar::Bink foo::Bar;
type foo::Coconut map;

; types egg::plant foo::bar;

; types banana import /foo/bar;

; types orange import /foo/bar forwarding {
; 	reset //;
; 	reset //foo/bar/baz;
; 	//banana/slop /wanana/bop;
; }

; mod eggplant import //foo/plant;

; mod eggplant import //foo/plant forwarding {
; 	//pear /foo;
; }

; todo why is eggplant2 not working?

; "Compiler" will be in-built type to make life easier

type foo::TestingSomePointers u8;

type TestingSomePointers u8;

type TesingPointers *u8;

type TestingPointersMore **u8;

; types compiler import //std/compiler;

; @Mod:ct compiler::CompTime;

; create(:ct compiler::CompTime, :argc usize, :argv **u8) -> {
; 	:ct ct;
; }

create() -> { ; this comment works
	test := 10;
	banana := u64 10;
	str := "Hello, Universe!";
	str2 := *u8 "Hello, Kingdom!";
	str3 := str2;
	
	log("Hello, World!");
	log();
	log("This log statement can now handle strings coming from variables.");
	log(str3);
}
