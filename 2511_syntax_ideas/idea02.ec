
; i was leaning towards not having special treatment for module instances and that we can have a receiver-like system for any sort of instances.
; but we would force that modules require an instance, in the sense that we have leaf maps that can't do much by themselves.

; it just could get messy if a module has the ability to create a vector etc.
; i might go with implicit module receivers (for anything called create_mod)?
; so instead of this:mod:blah we'd just have mod:blah.
; or maybe module gets special treatement i don't know at this point.

create(:my_dependency Foo, :math Math) {
	this:my_dependency @= my_dependency; // @= says "declare and assign if not already declared", whereas ":= is for variables and can have shadowing, whereas @= we know like there's genuinely only one version".
	this:math @= math;
}

@:create_world() {
	mod:my_dependency:do_something;
}

@:create_world:create_vector(x f64, y f64, z f64) {
	mod:my_dependency:do_something_or_another;
	
	this:x @= x;
	this:y @= y;
	this:z @= z;
}

@:create_world:create_vector:get_x() -> f64 {
	return this:x;
}

@:create_world:create_vector:get_y() -> f64 {
	return this:y;
}

@:create_world:create_vector:get_z() -> f64 {
	return this:z;
}

@:create_world:create_vector:get_distance(other T:create_world:create_vector) -> f64 {
	return mod:math:sqrt(
		+ (this:x - other:get_x) * (this:x - other:get_x)
		+ (this:y - other:get_y) * (this:y - other:get_y)
		+ (this:z - other:get_z) * (this:z - other:get_z)
	);
}

; i can talk about how forcing no ability for global variables forces the compiler to track everything. preparing us nicely for optimizations.

; need some self-loop of wanting to actually get the exposed version of "this", like "this~" or something.

; ah what now about private functions within my shtuff! now i need a symbol for private too?

; also look now carefully how awful the typing syntax is...
; maybe i do need to have type for returned instances, but god dammit i didn't want this.
; also note that i want want different factories for a type, but really the other factories should wrap a main internal factory kind of.

; also create_vector doesn't know any context about create_world unless the information of world is stored in vector's this... so it's kinda like not really appropriate to even have this create_world mentioned...

; note also that maybe if we do have like a Vector declaration that we need only include mutable things in here (to prevent the need for @=) and anything constant can be extended in the same way methods are.
