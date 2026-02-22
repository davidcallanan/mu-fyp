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
type Banana f64;
type foo::bar::Baz Banana;
type foo::bar::Bink foo::Bar;
type foo::Coconut {};

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

type String *u8;

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
	str4 := String "Test";
	mut str5 := String "Mutable String";
	
	object := {
		name := "David";
		age := 23;
		:octopus "Test";
		:another_variable 23;
		:name2 name;
		name3 := name2;
		:nested {
			:octopus "This would be really cool if it worked.";	
		};
	};
	
	foo := object:name2;
	bar := object:nested:octopus;
	
	log("Hello, World!");
	log();
	log("This log statement can now handle strings coming from variables.");
	log(str3);
	log(str4);
	log(foo);
	log(bar);
	log(access_during_assign := "This is really cool");
	
	log("Str 5 is:");
	log(str5);
	str5 = "New mutated value!";
	log("Str 5 is now:");
	log(str5);
	
	; for {
	;	log("Forever.");
	; }
}
