#include "total_needed_bits_multi.hpp"

#include <cstdint>
#include <numeric>
#include <vector>

// i think this implementation is right, but this is not a major concern for me.

uint32_t total_needed_bits_multi(const std::vector<uint32_t>& operand_bits) {
	return std::accumulate(operand_bits.begin(), operand_bits.end(), (uint32_t) 0);
}
