#pragma once

#include "maindef.h"
#include "packet_types.h"

struct SnapshotObject {
	id_t id;
	glm::vec2 position;
	int state;

	SnapshotObject(id_t _id, const glm::vec2& _position, int _state)
		: id(_id), position(_position), state(_state) {}
};

template <id_t ServerSize>
class SnapshotBuffer {
private:
	char* buffer;
	SnapshotObject* objects;
	id_t current;

	SnapshotBuffer(const SnapshotBuffer&) = delete;
	SnapshotBuffer(SnapshotBuffer&&) = delete;

public:
	SnapshotBuffer();
	~SnapshotBuffer();

	static constexpr id_t capacity() { return ServerSize; }
	static constexpr size_t capacityBytes();
	id_t size() const { return current; }
	size_t sizeBytes() const;

	void setHeader(const ServerSnapshotHeader& header);
	void add(const SnapshotObject& object);
	void reset() { current = 0; }

	char* getData() const { return buffer; }
	const ServerSnapshotHeader* getHeader() const { return (ServerSnapshotHeader*)buffer; }
	SnapshotObject* get(id_t index) const;

	inline SnapshotObject* operator[](id_t index) const { return get(index); }
};

template <id_t ServerSize>
constexpr size_t SnapshotBuffer<ServerSize>::capacityBytes() {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * ServerSize;
}

template <id_t ServerSize>
size_t SnapshotBuffer<ServerSize>::sizeBytes() const {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * current;
}

template <id_t ServerSize>
SnapshotBuffer<ServerSize>::SnapshotBuffer() : current(0) {
	buffer = new char[capacityBytes()];
	objects = (SnapshotObject*)(buffer + sizeof(ServerSnapshotHeader));
}

template <id_t ServerSize>
SnapshotBuffer<ServerSize>::~SnapshotBuffer() {
	delete[] buffer;
}

template <id_t ServerSize>
void SnapshotBuffer<ServerSize>::setHeader(const ServerSnapshotHeader& header) {
	memcpy(buffer, &header, sizeof(header));
}

template <id_t ServerSize>
void SnapshotBuffer<ServerSize>::add(const SnapshotObject& object) {
	if (current >= ServerSize) throw std::runtime_error("SnapshotBuffer index out of bounds!");
	objects[current] = object;
	current++;
}

template <id_t ServerSize>
SnapshotObject* SnapshotBuffer<ServerSize>::get(id_t index) const {
	return objects + index;
}
