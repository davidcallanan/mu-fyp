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
@MyService:description "This is a really cool service";

@MyService:print_foo input {
	:foo u64;
} mut -> {
	log_d(input:foo);
	log("I also know my description:");
	log(this:description);
	log("I also know the module's description:");
	log(mod:description);
	log("My mutated description is:");
	this:description = "This is a mutated description.";
	log(this:description);
};

@Mod:name "My Module";
@Mod:description "This is a fantastic description..";

@Mod:do_something input {} -> {
	log("Doing something...");
	log("I also can access \"this\" and \"mod\"...");
	log(this:name);
	log(mod:name);
};

; @Mod:my_printf extern ccc "printf" {
; 	:message *u8;
; } -> {};

@Mod:my_printf extern ccc "printf" (*u8) -> {};

@Mod:my_getpid extern ccc "getpid" () -> (i32);

type Return (u64);

@Mod:nested_function () -> Return {
	:0 12;
};

@Mod:new_function () -> Return {
	tmp := this:nested_function():0;
	:0 tmp;
};

create extern ccc "main" () -> { ; this comment works
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
	
	log("Dynamic sym call:");
	sym_of_interest := :star_sign;
	log_d(object sym_of_interest);
	
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
	
	actually_callable := {
		input {
			:foo u64;
		} -> {
			log_d(input:foo);
		}
	};
	
	actually_callable {
		:foo 123;
	};
	
	syntactically_shorter := input {
		:foo u64;
	} -> {
		log_d(input:foo);
	};
	
	syntactically_shorter {
		:foo 456;
	};
	
	syntactically_shorter {
		:foo 567;
	};
	
	x := MyService;
	
	log(x:name);
	
	x:print_foo {
		:foo 678;
	};
	
	log(mod:name);
	
	mod:do_something {};
	
	; mod:my_printf {
	; 	:message "Test\n";
	; };
	
	mod:my_printf("\"Test\" \\ \\\n");
	
	mod:my_printf {
		:0 "\"Test\" \\ \\\n";
	};
	
	mod:my_printf alwaysinline {
		:0 "Inlined!\n";
	};

	pid := mod:my_getpid();
	
	log("PID is:");
	log_d(pid:0);
	
	y := &MyService;
	
	log(y:name);
	log(y:description);
	
	y:print_foo {
		:foo 678;
	};
	
	log_d(sizeof(x));
	log_d(sizeof(y));
	
	log("New operations:");
	
	w := 23 b& 41;
	w2 := (: u64 19) << 2;
	w3 := bool :true;
	w4 := !w3;
	
	log_d(w);
	log_d(w2);
	log_d(w3);
	log_d(w4);
	
	log("Testing return values:");
	
	log_d(mod:new_function():0);
	
	x := &mut {
		:name "David";	
	};
	
	log("Mutation test:");
	
	log(x:name);
	
	x:name = "John Doe";
	
	log(x:name);
}
