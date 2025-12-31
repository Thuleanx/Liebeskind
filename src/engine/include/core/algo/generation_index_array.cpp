#include "core/logger/assert.h"

namespace algo {

template <size_t N>
GenerationIndexArray<N> GenerationIndexArray<N>::create() {
	const std::vector<uint16_t> free = [&]() {
		std::vector<uint16_t> result;
		result.reserve(N);
		for (uint16_t i = 0; i < N; i++) result.push_back(i);
		return result;
	}();
    return GenerationIndexArray<N>(free);
}

template <size_t N>
GenerationIndexArray<N>::GenerationIndexArray(
	std::vector<uint16_t> free
) :
	free(free) {}

template <size_t N>
GenerationIndexPair reserveIndex(GenerationIndexArray<N>& array) {
	ASSERT(
		array.free.size(),
		"Exceeding the capacity of the generation index array (" << N << ")"
	);
	const uint16_t index = array.free.back();
	array.free.pop_back();
	const uint16_t generation = array.generation[index];
	return {
		.index = index,
		.generation = generation,
	};
}

template <size_t N>
std::vector<uint16_t> getLiveIndices(const GenerationIndexArray<N>& array) {
	std::array<bool, N> isFree;
	std::fill(isFree.begin(), isFree.end(), false);
	for (const int& freedIndex : array.free) isFree[freedIndex] = true;
	std::vector<uint16_t> liveIndices;
	liveIndices.reserve(N - array.free.size());
	for (size_t i = 0; i < N; i++) {
		if (!isFree[i]) liveIndices.push_back(i);
	}
	return liveIndices;
}

template <size_t N>
void destroy(
	GenerationIndexArray<N>& array, std::span<const GenerationIndexPair> indices
) {
	for (const GenerationIndexPair& index : indices) {
		ASSERT(isIndexValid(array, index), "Cannot destroy an invalid entry");
		array.generation[index.index]++;
		array.free.push_back(index.index);
	}
}

}  // namespace algo
