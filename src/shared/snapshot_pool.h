#pragma once

#include "maindef.h"
#include "snapshot_buffer.h"

#include <array>
#include <optional>

struct SnapshotPair {
	SnapshotBuffer a, b;
	float t;

	SnaphostPair(char* aBuffer, char* bBuffer, float tKoeff) : a(aBuffer), b(bBuffer), t(tKoeff) {}
};

template <size_t Size>
class SnapshotPool {
private:
	id_t snapshotSize;
	size_t current;
	std::array<char*, Size> buffers;

public:
	static constexpr time_t INTERPOLATION_OFFSET =  100;

	SnapshotPool();
	SnapshotPool(id_t snapshotSize);
	~SnapshotPool();

	size_t size() const { return current; }
	void push(const SnapshotBuffer& snapshot);
	void push(char* rawBuffer);
	void reset() { current = 0; }

	std::optional<SnapshotPair> getInterpolationPair(time_t now);
};

template <size_t Size>
SnapshotPool<Size>::SnapshotPool() : snapshotSize(0), current(0) {
	if (!buffers[0]) std::println("Buffer is already nullptr");
	memset(&buffers[0], NULL, Size);
}

template <size_t Size>
SnapshotPool<Size>::SnapshotPool(id_t _snapshotSize) : snapshotSize(_snapshotSize), current(0) {
	for (char* buffer : buffers) {
		if (buffer) delete[] buffer;
		buffer = new char[snapshotSize];
	}
}

template <size_t Size>
SnapshotPool<Size>::~SnapshotPool() {
	for (char* buffer : buffers) delete[] buffer;
}

template <size_t Size>
void SnapshotPool<Size>::push(const SnapshotBuffer& snapshot) {
	std::println("Pushing at {}", current);
	memcpy(buffers[current], snapshot.getData(), current);
	current = (current + 1) % Size;
}

template <size_t Size>
void SnapshotPool<Size>::push(char* buffer) {
	memcpy(buffers[current], buffer, current);
	current = (current + 1) % Size;
}

template <size_t Size>
std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
	if (size() < 2) { std::nullopt; }

	time_t shifted = now - INTERPOLATION_OFFSET;
	size_t fromIndex = 0, toIndex = size() - 1;

	for (size_t i = size() - 1; i >= 0; i--) {
		SnapshotBuffer snapshot(buffers[i]);
		if (shifted - snapshot.getHeader()->getTimestamp() > 0) {
			fromIndex = i;
			snapshot.invalidate();
			break;
		}
		snapshot.invalidate();
	}

	for (size_t i = 0; i < size(); i++) {
		SnapshotBuffer snapshot(buffers[i]);
		if (shifted - snapshot.getHeader()->getTimestamp() < 0) {
			toIndex = i;
			snapshot.invalidate();
			break;
		}
		snapshot.invalidate();
	}

	if (fromIndex == toIndex) {
		if (toIndex == 0) return SnapshotPair(buffers[0], buffers[0], 1.0f);
		if (toIndex == size() - 1) return std::nullopt;
	}
	reset();
	
	SnapshotPair pair(buffers[fromIndex], buffers[toIndex], 0.0f);
	time_t from = pair.a.getHeader()->getTimestamp();
	time_t to = pair.b.getHeader()->getTimestamp();
	pair.t = (float)(shifted - from) / (float)(to - from);

	return pair;
}