#pragma once

#include "maindef.h"
#include "snapshot_buffer.h"
#include <array>
#include <optional>

struct SnapshotPair {
	SnapshotBuffer a, b;
	float t;

	SnapshotPair(char* aBuffer, char* bBuffer, float tKoeff)
		: a(aBuffer), b(bBuffer), t(tKoeff) {}

	~SnapshotPair() {}
};

template <size_t Size>
class SnapshotPool {
private:
	id_t snapshotSize, snapshotSizeBytes;
	size_t current, maximal;
	bool initialized;
	std::array<char*, Size> buffers;

public:
	static constexpr time_t INTERPOLATION_OFFSET = 60000;

	SnapshotPool();
	~SnapshotPool();

	size_t size() const { return current; }

	void init(id_t snapshotSize);
	void push(const SnapshotBuffer& snapshot);
	void push(char* rawBuffer);
	void reset() { current = 0; }

	std::optional<SnapshotPair> getInterpolationPair(time_t now);
};

template <size_t Size>
SnapshotPool<Size>::SnapshotPool()
	: snapshotSize(0), snapshotSizeBytes(0), current(0), maximal(0), initialized(false) {
	if (!buffers[0]) std::println("Buffer is already nullptr");
	std::println("SnapshotPool's default constructor!");

	for (size_t i = 0; i < Size; i++) buffers[i] = nullptr;
}

template <size_t Size>
SnapshotPool<Size>::~SnapshotPool() {
	for (char* buffer : buffers) delete[] buffer;
}

template <size_t Size>
void SnapshotPool<Size>::init(id_t snapshotSize) {
	if (initialized) return;
	this->snapshotSize = snapshotSize;
	snapshotSizeBytes = SnapshotBuffer::sCapacityBytes(snapshotSize);

	std::println("Allocating pool buffers");
	for (size_t i = 0; i < Size; i++) {
		buffers[i] = new char[snapshotSizeBytes];
	}

	initialized = true;
}

template <size_t Size>
void SnapshotPool<Size>::push(const SnapshotBuffer& snapshot) {
	//std::println("Pushing at {}", current);
	memcpy(buffers[current], snapshot.getData(), snapshotSizeBytes);
	if (maximal < Size - 1) maximal = current;
	current = (current + 1) % Size;
}

template <size_t Size>
void SnapshotPool<Size>::push(char* buffer) {
	//std::println("Pushing at {}", current);
	memcpy(buffers[current], buffer, snapshotSizeBytes);
	//SnapshotBuffer snapshot(buffer);
	//std::println("Dumping currently pushed:");
	//snapshot.dump();

	if (maximal < Size - 1) maximal = current;
	current = (current + 1) % Size;
}

//template <size_t Size>
//std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
//	if (size() < 2) { 
//		//std::println("Case 1");
//		return std::nullopt;
//	}
//
//	time_t shifted = now - INTERPOLATION_OFFSET;
//	size_t fromIndex = size() - 1, toIndex = 0;
//
//	for (size_t i = size(); i-- > 0;) {
//		SnapshotBuffer snapshot(buffers[i]);
//		if (shifted - snapshot.getHeader()->getTimestamp() > 0) {
//			fromIndex = i;
//			snapshot.invalidate();
//			break;
//		}
//		snapshot.invalidate();
//	}
//
//	MY_ASSERT(buffers[0] != nullptr, "buffers[0] was nullptr!");
//
//	for (size_t i = 0; i < size(); i++) {
//		SnapshotBuffer snapshot(buffers[i]);
//		if (shifted - snapshot.getHeader()->getTimestamp() < 0) {
//			toIndex = i;
//			snapshot.invalidate();
//			break;
//		}
//		snapshot.invalidate();
//	}
//
//	if (fromIndex == toIndex) {
//		MY_ASSERT(buffers[0] != nullptr, "buffers[0] was nullptr!");
//		if (toIndex == 0) {
//			//std::println("Case 2");
//			return std::make_optional<SnapshotPair>(buffers[0], buffers[0], 1.0f);
//		}
//		if (toIndex == size() - 1) {
//			//std::println("Case 3, toIndex: {0}, size: {1}", toIndex, size());
//			return std::nullopt;
//		}
//	}
//	//reset();
//	
//	std::optional<SnapshotPair> pair(std::in_place, buffers[fromIndex], buffers[toIndex], 0.0f);
//	time_t from = pair->a.getHeader()->getTimestamp();
//	time_t to = pair->b.getHeader()->getTimestamp();
//	pair->t = (float)(shifted - from) / (float)(to - from);
//
//	//std::println("from={}, to={}, t={}, shifted={}, now={}",
//	//	fromIndex, toIndex, pair->t, shifted, now);
//
//	//std::println("Case 4");
//	return pair;
//}

template <size_t Size>
std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
	if (maximal < 2) {
		std::println("Case 1. Maximal = {}", maximal);
		return std::nullopt;
	}

	char* from = nullptr;
	char* to = nullptr;
	time_t shifted = now - INTERPOLATION_OFFSET;

	for (size_t i = 0; i < maximal; i++) {
		SnapshotBuffer snapshot(buffers[i]);
		if (snapshot.getHeader()->getTimestamp() <= shifted) {
			from = buffers[i];
		}
		else if (!to) {
			to = buffers[i];
		}
		snapshot.invalidate();
	}

	if (!from || !to) {
		std::println("Case 2");
		return std::nullopt;
	}
	
	std::optional<SnapshotPair> pair(std::in_place, from, to, 0.0f);
	time_t timeFrom = pair->a.getHeader()->getTimestamp();
	time_t timeTo = pair->b.getHeader()->getTimestamp();
	pair->t = (float)(shifted - timeFrom) / (float)(timeTo - timeFrom);

	//std::println("from={}, to={}, t={}, shifted={}, now={}",
	//	fromIndex, toIndex, pair->t, shifted, now);

	return pair;
}