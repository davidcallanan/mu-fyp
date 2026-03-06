#include "destroy_dummy_igc.hpp"

void destroy_dummy_igc(DummyIgc& dummy) {
	dummy.dummy_func->eraseFromParent();
}
