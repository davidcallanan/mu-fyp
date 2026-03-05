
forwarding {
	//banana/slop /foo/bop;
	//banana/vegetable /foo/gravy;
}

type Bar {};
type Car {};

@Bar:test:blah27 f32;

type blah::Wah {};

@Bar:foo blah::Wah;

type NewType f32 {}
; type NewType map {}

type NewTypeAgain f32 5.0;
; type NewTypeAgain map 5;

type AnotherNewType f32 5.0;
type AnotherNewTypeAlright 5;

type Blah 5.5;

@Car:why_is_this_disappearing i32;
@Car:testing f32 5.0;
; @Car:testing 5;
