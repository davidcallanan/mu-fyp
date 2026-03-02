#include "total_needed_bits_addit.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

// this uses a logarithmic approach (well, sort of, but by using hardware floats) to fit it into a double.
// this avoids needing a "BigInt" type in C++.
// idea inspired from LLM, see appendix.

uint32_t total_needed_bits_addit(const std::vector<uint32_t>& bits) {
	if (bits.empty()) {
		return 0;
	}

	double max_sum = 0.0;

	for (const uint32_t bit : bits) {
		max_sum += std::pow(2.0, (double) bit) - 1.0;
	}

	if (max_sum <= 0.0) {
		return 0;
	}

	double my_epsilon = 1e-9; // in case precision is lost, we need to be a tiny bit more conservative.
	
	return (uint32_t) std::floor(std::log2(max_sum) + my_epsilon) + 1;
}
