#pragma once

#include "x86_64/port_manager.h"

struct Mod {
	// use of a pointer simulates being able to substitute the implementation at runtime.
	struct PortManager* port_manager;
};

struct Mod* mod__create();
