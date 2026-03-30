#pragma once

#include "maindef.h"
#include "snapshot_buffer.h"

#include <array>
#include <optional>

struct SnapshotPair {
	SnapshotBuffer a, b;
	time_t delay;

	SnaphostPair(char* aBuffer, char* bBuffer, time_t _delay)
	:	a(aBuffer), b(bBuffer), delay(_delay) {}
};

template <size_t Size>
class SnapshotPool {
private:
	id_t snapshotSize;
	size_t current;
	std::array<char*, Size> buffers;

public:
	static constexpr time_t INTERPOLATION_DELAY = 5;

	SnapshotPool(id_t snapshotSize);
	~SnapshotPool();

	size_t size() const { return current; }
	void push(const SnapshotBuffer& snapshot);
	std::optional<SnapshotPair> getInterpolationPair(time_t now) const;
};

template <size_t Size>
SnapshotPool<Size>::SnapshotPool(id_t _snapshotSize) : snapshotSize(_snapshotSize), current(0) {
	for (char* buffer : buffers) buffer = new char[snapshotSize];
}

template <size_t Size>
SnapshotPool<Size>::~SnapshotPool() {
	for (char* buffer : buffers) delete[] buffer;
}

template <size_t Size>
void SnapshotPool<Size>::push(const SnapshotBuffer& snapshot) {
	memcpy(buffers[current], snapshot.getData(), current);
	current = (current + 1) % Size;
}

template <size_t Size>
std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair() const {
	if (size() < 2) { std::nullopt; }

	return std::nullopt;
}