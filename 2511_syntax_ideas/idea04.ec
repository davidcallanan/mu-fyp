; the goal of this iteration is to determine how public private should work
; and to resolve the notations of maps (RHS) of @ extensions.
; i'd also like to determine how to make things mutable.

; anything mutable must be defined in the type. how about we suppose that this is not necessary for sake of exploration.

@Mod:my_dependency Foo;
@Mod:math Math;

create(:my_dependency Foo, :math Math) {
	mod:my_dependency = my_dependency;
	mod:math = math;
}

type World map;

pub @Mod:create_world map () -> World . { ; . means implicit? this distinguishes between type details and actual body.
	mod:my_dependency:do_something;
}

type Vector map;

mut @Vector:x f64; mut kinda means non-deterministic
mut @Vector:y f64; these should mutable because maybe we have a method to mutate them
mut @Vector:z f64;

pub @World:create_vector map (x f64, y f64, z f64) -> Vector . {
	mod:my_dependency:do_something_or_another;
	
	; this is problematic. this could be referring to world, or it could be referring to vector instance.
	
	this:x = x;
	this:y = y;
	this:z = z;
}

pub @Vector:get_x map . {
	return this:x;
}

pub @Vector:get_y map . {
	return this:y;
}
	
pub @Vector:get_z map . {
	return this:z;
}

pub @Vector:get_distance map (other Vector) -> f64 . {
	return mod:math:sqrt(
		+ (this:x - other:get_x) * (this:x - other:get_x)
		+ (this:y - other:get_y) * (this:y - other:get_y)
		+ (this:z - other:get_z) * (this:z - other:get_z)
	);
}

; on the next iteration i need to resolve some things:
; - this does not work in factories very nicely... ahhh!!!
; - . for implicit is weird... i need to think that through more.
