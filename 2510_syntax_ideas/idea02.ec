; this is a comment because why not! i think it would be pretty cool.
; double slash means go to parent root
; triple slash means go to parent root and find its parent root
; there is a cap on triple slash. if you need to go further, you must then start doing mappings within the parent root "sources" entry.
; this ensures we dont go absolutely crazy.

reset-double-root; this resets // to current module
reset-triple-root; this resets /// to current module

sources {
	/// / ; maybe this is a better way to reset?
	// /
	/// reset; or reset like this
	/deps/foo //deps/foo
	/deps/bar ///deps/bar
	print("hello");
	; only sources can access // and ///, using this anywhere else would be illegal, so you have to create a local mapping.
	// reset; must come at the end because previous mappings will refer to old version... or not, the user can just have the common sense to know that this only impacts child modules.
	/// resetchildren; this resets only for children.
	; or preferably
	double-reset;
	triple-reset;
	triple-reset-children;
	double-reset-children;
	; or maybe
	reset-double;
	reset-triple;
	reset-triple-children;
	reset-double-children;
}
