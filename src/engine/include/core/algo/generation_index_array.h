#pragma once

#include <array>
#include <span>
#include <vector>

#include "core/logger/assert.h"

namespace algo {

struct GenerationIndexPair {
	uint16_t index;
	uint16_t generation;

   public:
	bool operator<(const GenerationIndexPair& other) const {
		return index < other.index;
	}
	bool operator==(const GenerationIndexPair& other) const {
		return index == other.index && generation == other.generation;
	}
};

template <size_t N>
struct GenerationIndexArray {
	std::array<uint16_t, N> generation;
	std::vector<uint16_t> free;

	static_assert(N <= (1 << 16), "N is too large, keep it under 2^{16}");

   public:
	GenerationIndexArray() = delete;
	static GenerationIndexArray create();

   private:
	GenerationIndexArray(std::vector<uint16_t> free);
};

template <size_t N>
[[nodiscard]]
GenerationIndexPair reserveIndex(GenerationIndexArray<N>& array);

template <size_t N>
std::vector<uint16_t> getLiveIndices(const GenerationIndexArray<N>& array);

template <size_t N>
inline bool isIndexValid(
	const GenerationIndexArray<N>& array, GenerationIndexPair index
) {
	ASSERT(
		index.index < N,
		"Index " << index.index << " exceeds the capacity of " << N
	);
	return array.generation[index.index] == index.generation;
}

template <size_t N>
void destroy(
	GenerationIndexArray<N>& array, std::span<const GenerationIndexPair> indices
);

}  // namespace algo

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused"
#include "generation_index_array.cpp"
#pragma GCC diagnostic pop
