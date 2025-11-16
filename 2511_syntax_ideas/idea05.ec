; i think we already decided prior that the whole point of prefixing maps with "map" at the type level is so that we can take advantage of () and {} notation for instances.

; so i wonder can we get going with that immediately.

@Mod:my_dependency Foo;
@Mod:math Math;

create(:my_dependency Foo, :math Math) {
	:my_dependency my_dependency;
	:math math;
}

type World map;

pub @Mod:create_world() -> World { ; here World is kinda optional, and we can make everything optional because we know the difference between type and instance at the lexer level almost.
	mod:my_dependency:do_something;
}

type Vector map;

mut @Vector:x f64;
mut @Vector:y f64;
mut @Vector:z f64;

pub @World:create_vector(:x f64, :y f64, :z f64) -> Vector {
	mod:my_dependency:do_something_or_another;
	
	; normally we would return things like this:
	
	:x x;
	:y y;
	:z z;
	
	; but remember! these are private!
	; seems dodgy.
	; then again, the notion of private is a module-level thing (Go ftw).
	; there's not supposed to be the ability to do private.
	; if you need truly private and temporary, then use a local variable
	; otherwise you just gotta use a field.
	; you can internally decide on a _x convention if wanted, but my thinking is that this is unnecessary
	; since i myself have accessed _x things outside where i thought i would in javascript many times, that's the whole point of having a module level of privacy.
}

pub @Vector:get_x f64 {
	return this:x;
}

pub @Vector:get_y f64 {
	return this:y;
}
	
pub @Vector:get_z f64 {
	return this:z;
}

pub @Vector:get_distance(other Vector) -> f64 {
	return mod:math:sqrt(
		+ (this:x - other:get_x) * (this:x - other:get_x)
		+ (this:y - other:get_y) * (this:y - other:get_y)
		+ (this:z - other:get_z) * (this:z - other:get_z)
	);
}

; this actually looks quite good.
