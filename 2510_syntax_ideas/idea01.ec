what i want to do is write my own language, but allow to user to link in with llvm ir should any functionality not be feasible in my language (e.g. specific optimization flags etc).

sources :intf {
	/x/y/z /blah/wah/frah
}

sources :impl {
	/x/y/z /blah/wah/frah
}

deps {
	:print (/print, :static(/x86_64/print, :object_wide:global)); // uses one inter-object implementation
	:print (/print, :static(/x86_64/print, :object_wide:named(/my_lovely_instance)));
	:print (/print, :static(/x86_64/print, :local)); // any instance of this module has its own implementation
	:print (/print :)
}

no because i think the implementation of being static etc actually depends on the bigger picture

deps {
	:print (/print, :concrete(/x86_64/print, :local, :static));
	:print (/print, :concrete(/x86_64/print, :named(/my_implementation), :static));
	:print (/print, :passed:named(/my_implementation));
}

# this is tough

maybe a distinction for each "module" of what's "internal" to that module and what is "external". anything internal it is the responsibility of the module author to instantiate one with internal aspects prepopulated.

in another module, loading up a module it is assumed that anything internal is pre-populated.

when something is internal it is understood that you cannot create "multiple instances" (not of the module but of the module factory i mean). thus, that module can be configured at a global level.

instantiate modsource ();

:x86_64:print

do i really need a / variant?

print := import_intf:print;

the idea is that / is subject to translation.

any functions that work on the same "realm" will automatically use the designated translator when taking input. [for now, this translator is not going to exist, but the point is it could eventually].


