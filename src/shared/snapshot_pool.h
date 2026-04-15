#pragma once

#include "maindef.h"
#include "snapshot_buffer.h"
#include <array>
#include <string>
#include <optional>

struct SnapshotPair {
	SnapshotBuffer a, b;
	float t;

	SnapshotPair(char* aBuffer, char* bBuffer, float tKoeff)
		: a(aBuffer), b(bBuffer), t(tKoeff) {}

	~SnapshotPair() {}
};

template <size_t Val>
concept CoilPoolAcceptable = (Val >= 2);

#define TEMPLATE_SNAPSHOT_POOL template <size_t Size> requires CoilPoolAcceptable<Size>

TEMPLATE_SNAPSHOT_POOL
class SnapshotPool {
private:
	id_t snapshotSize, snapshotSizeBytes;
	size_t head, tail, maximal;
	bool initialized;
	std::array<char*, Size> buffers;

	void moveWindow();
	size_t coilUp(size_t n) { return (n + 1) % Size; }
	size_t coilDown(size_t n) { return (n - 1) % Size; }

public:
	static constexpr time_t INTERPOLATION_OFFSET = 60000;

	SnapshotPool();
	~SnapshotPool();

	size_t getTail() const { return tail; }
	size_t getHead() const { return head; }
	size_t size() const { return maximal; }

	bool isInitialized() const { return initialized; }

	void init(id_t snapshotSize);
	void push(const SnapshotBuffer& snapshot);
	void push(char* rawBuffer);
	void reset() { tail = 0; }

	std::optional<SnapshotPair> getInterpolationPair(time_t now);
};

TEMPLATE_SNAPSHOT_POOL
SnapshotPool<Size>::SnapshotPool()
	: snapshotSize(0), snapshotSizeBytes(0), head(0), tail(0), maximal(0), initialized(false) {
	if (!buffers[0]) std::println("Buffer is already nullptr");
	std::println("SnapshotPool's default constructor!");

	for (size_t i = 0; i < Size; i++) buffers[i] = nullptr;
}

TEMPLATE_SNAPSHOT_POOL
SnapshotPool<Size>::~SnapshotPool() {
	for (char* buffer : buffers) delete[] buffer;
}

TEMPLATE_SNAPSHOT_POOL
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

TEMPLATE_SNAPSHOT_POOL
void SnapshotPool<Size>::moveWindow() {
	tail = coilUp(tail);
	if (head == tail) head = coilUp(head);
	maximal = std::min(maximal + 1, Size);
}

TEMPLATE_SNAPSHOT_POOL
void SnapshotPool<Size>::push(const SnapshotBuffer& snapshot) {
	//std::println("Pushing at {}", current);
	memcpy(buffers[tail], snapshot.getData(), snapshotSizeBytes);
	moveWindow();
}

TEMPLATE_SNAPSHOT_POOL
void SnapshotPool<Size>::push(char* buffer) {
	//std::println("Pushing at {}", current);
	memcpy(buffers[tail], buffer, snapshotSizeBytes);
	//SnapshotBuffer snapshot(buffer);
	//std::println("Dumping currently pushed:");
	//snapshot.dump();

	moveWindow();
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

TEMPLATE_SNAPSHOT_POOL
std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
	if (size() < 2) {
		//std::println("Case 1. Maximal = {}", maximal);
		return std::nullopt;
	}

	time_t shifted = now - INTERPOLATION_OFFSET;
	char* from = nullptr;
	char* to = nullptr;
	size_t fromIndex = 0, toIndex = 0, 
		actualTail = size() == Size ? tail : tail - 1; // !!!

	//std::println("actualTail = {}", actualTail);

	size_t i = head;
	std::string test = "";
	do {
		SnapshotBuffer snapshot(buffers[i]);
		time_t timestamp = snapshot.getHeader()->getTimestamp();
		
		if (timestamp <= shifted) {
			from = buffers[i];
			fromIndex = i;
		}
		else if (!to) {
			to = buffers[i];
			toIndex = i;
		}

		i = coilUp(i);
		test += std::to_string(i) + " ";
	} while (i != actualTail);
	//std::println("{0} -> {1} ({2}) | {3}", head, tail, actualTail, test);

	if (!from) {
		//std::println("Case 2");
		return std::nullopt;
	}
	if (!to) {
		//std::println("Case 3");
		return std::make_optional<SnapshotPair>(from, from, 1.0f);
	}

	std::optional<SnapshotPair> pair(std::in_place, from, to, 0.0f);
	time_t timeFrom = pair->a.getHeader()->getTimestamp();
	time_t timeTo = pair->b.getHeader()->getTimestamp();
	pair->t = (float)(shifted - timeFrom) / (float)(timeTo - timeFrom);

	//std::println("from={}, to={}, t={}, maximal={}",
	//	fromIndex, toIndex, pair->t, maximal);

	return pair;
}

//template <size_t Size>
//std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
//	if (maximal < 2) {
//		std::println("Case 1. Maximal = {}", maximal);
//		return std::nullopt;
//	}
//
//	char* from = nullptr;
//	char* to = nullptr;
//	size_t fromIndex = 0, toIndex = 0;
//
//	time_t shifted = now - INTERPOLATION_OFFSET;
//
//	for (size_t i = 0; i < maximal; i++) {
//		SnapshotBuffer snapshot(buffers[i]);
//		if (snapshot.getHeader()->getTimestamp() <= shifted) {
//			from = buffers[i];
//			fromIndex = i;
//		}
//		else if (!to) {
//			to = buffers[i];
//			toIndex = i;
//		}
//		snapshot.invalidate();
//	}
//
//	if (!from) {
//		std::println("Case 2");
//		return std::nullopt;
//	}
//	if (!to) {
//		std::println("Case 3");
//		return std::make_optional<SnapshotPair>(from, from, 1.0f);
//	}
//	
//	std::optional<SnapshotPair> pair(std::in_place, from, to, 0.0f);
//	time_t timeFrom = pair->a.getHeader()->getTimestamp();
//	time_t timeTo = pair->b.getHeader()->getTimestamp();
//	pair->t = (float)(shifted - timeFrom) / (float)(timeTo - timeFrom);
//
//	std::println("from={}, to={}, t={}, maximal={}",
//		fromIndex, toIndex, pair->t, maximal);
//
//	return pair;
//}

//template <size_t Size>
//std::optional<SnapshotPair> SnapshotPool<Size>::getInterpolationPair(time_t now) {
//	if (current == 0) {
//		std::println("Case 1. No snapshots yet");
//		return std::nullopt;
//	}
//
//	time_t shifted = now - INTERPOLATION_OFFSET;
//	size_t newest = (current + Size - 1) % Size;
//
//	char* from_buf = nullptr;
//	char* to_buf = nullptr;
//	time_t from_time = 0;
//	time_t to_time = 0;
//
//	for (size_t i = 0; i < Size; ++i) {
//		size_t idx = (newest + Size - i) % Size;
//		if (buffers[idx] == nullptr) continue;
//
//		SnapshotBuffer snap(buffers[idx]);
//		time_t ts = snap.getHeader()->getTimestamp();
//		snap.invalidate();
//
//		if (ts <= shifted) {
//			if (from_buf == nullptr) {
//				from_buf = buffers[idx];
//				from_time = ts;
//				break;  // нашли нужную пару, дальше не ебёмся
//			}
//		}
//		else {
//			to_buf = buffers[idx];
//			to_time = ts;
//		}
//	}
//
//	if (from_buf == nullptr) {
//		std::println("Case 2: no snapshot <= shifted");
//		return std::nullopt;
//	}
//
//	if (to_buf == nullptr) {
//		std::println("Case 3: only one snapshot, using t=1.0");
//		return std::make_optional<SnapshotPair>(from_buf, from_buf, 1.0f);
//	}
//
//	std::optional<SnapshotPair> pair(std::in_place, from_buf, to_buf, 0.0f);
//	pair->t = (to_time == from_time) ? 1.0f :
//		static_cast<float>(shifted - from_time) / static_cast<float>(to_time - from_time);
//
//	std::println("from_ts={}, to_ts={}, t={}, shifted={}", from_time, to_time, pair->t, shifted);
//	return pair;
//}