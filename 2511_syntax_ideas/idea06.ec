@Mod:my_dependency Foo;
@Mod:math Math;

create(:my_dependency Foo, :math Math) {
	:my_dependency my_dependency;
	:math math;
}

type World map;

pub @Mod:create_world() -> World {
	mod:my_dependency:do_something;
}

type Vector map;

mut @Vector:x f64;
mut @Vector:y f64;
mut @Vector:z f64;

pub @World:create_vector(:x f64, :y f64, :z f64) -> Vector {
	mod:my_dependency:do_something_or_another;
	
	:x x;
	:y y;
	:z z;
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
