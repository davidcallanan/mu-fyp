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

type MyService {};

@MyService:name "My Fantastic Service";

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
		:star_sign enum {
			:aries;
			:taurus;
			:capricorn;
		} :capricorn;
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
	
	log("Singletonish optimization test:");
	log_d(object:star_sign); // should be a constant
	log_d(object:another_variable); // should be a constant
	
	log("Str 5 is:");
	log(str5);
	str5 = "New mutated value! I hope this formats well. abc";
	log("Str 5 is now:");
	log(str5);
	
	; for {
	;	log("Forever.");
	; }
	
	advanced_calculation :=
		+ 50
		+ 70
		- 30
	;
	
	log_d(advanced_calculation);
	log_d(156);
	log_d(str5);
	log("null-term");
	log_dd(str5, null-term);
	log("byte-count");
	log_dd(str5, 20);
	log("byte-count");
	log_dd(str5, 5);
	
	boolean := :true;
	type_ := :foo_bar_baz;
	
	log("Enum dump:");
	log_d(boolean);
	log_d(type_);
	
	new_boolean := enum {
		:false;
		:true;
	} :true;
	
	log_d(new_boolean);
	
	log("Boolean values:");
	 
	mut actual_boolean := bool :true;
	
	log_d(actual_boolean);
	
	actual_boolean = bool :false;
	
	log_d(actual_boolean);
	
	logical_test_a :=
		|| actual_boolean
		|| bool :true
	;
	
	logical_test_b := 
		&& actual_boolean
		&& bool :true
	;
	
	logical_test_c := 
		&& bool :true
		&& bool :true
	;
	
	log("Logical tests:");
	
	log_d(logical_test_a);
	log_d(logical_test_b);
	log_d(logical_test_c);
	
	if (bool :true) {
		log("Scenario 1A");
	} else if (bool :false) {
		log("Scenario 1B");
	} else {
		log("Scenario 1C");
	}
	
	
	if (bool :false) {
		log("Scenario 2A");
	} else if (bool :true) {
		log("Scenario 2B");
	} else {
		log("Scenario 2C");
	}
	
	if (bool :false) {
		log("Scenario 3A");
	} else if (bool :false) {
		log("Scenario 3B");
	} else {
		log("Scenario 3C");
	}
	
	mut i := u64 0;
	
	for {
		log("Hello from a loop.");
		
		i = i + 1;
		
		log_d(i);
		
		if (i >= 10) {
			log("breaking...");
			break;
		}
	}
}
