; let's have another go at idea02.

type Mod map {
	:_my_dependency Foo;
	:_math Math;
}

create(:my_dependency Foo, :math Math) {
	; if constructor finds something not assigned, compile-time error.
	mod:_my_dependency = my_dependency;
	mod:_math = math;
	; i'm thinking of _ here but in practice i think i want to follow the public approach where there is no such thing as module-level private.
}

type World map {
}

; the idea is that one should be able to immediately start doing stuff with @Mod without even using "create" or "type Mod map" unless they actually need it.

@Mod:create_world() -> World { ; we want () here to allow later extensibility.. this is just a convention that programmers would likely follow.
	mod:_my_dependency:do_something;
}

type Vector map {
	:_x f64;
	:_y f64;
	:_z f64;
}

@World:create_vector(x f64, y f64, z f64) -> Vector {
	mod:_my_dependency:do_something_or_another;
	
	this:_x = x;
	this:_y = y;
	this:_z = z;
}

@Vector:get_x() -> f64 {
	return this:_x;
}

@Vector:get_y() -> f64 {
	return this:_y;
}

@Vector:get_z() -> f64 {
	return this:_z;
}

@Vector:get_distance(other T:Vector) -> f64 {
	return mod:_math:sqrt(
		+ (this:_x - other:get_x) * (this:_x - other:get_x)
		+ (this:_y - other:get_y) * (this:_y - other:get_y)
		+ (this:_z - other:get_z) * (this:_z - other:get_z)
	);
}
