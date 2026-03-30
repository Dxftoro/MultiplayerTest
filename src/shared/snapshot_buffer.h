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

/* Maybe try to make this as something called a "Snapshot" */
class SnapshotBuffer {
private:
	/* And this to be a "SnapshotBuffer" */
	char* buffer;
	SnapshotObject* objects;
	id_t current, _capacity;

	SnapshotBuffer(const SnapshotBuffer&) = delete;
	SnapshotBuffer(SnapshotBuffer&&) = delete;

	SnapshotObject* getFirstObject() const { return (SnapshotObject*)(buffer + sizeof(ServerSnapshotHeader)); }

public:
	explicit SnapshotBuffer(id_t capacity);
	explicit SnapshotBuffer(char* buffer);
	~SnapshotBuffer();

	id_t capacity() const { return _capacity; }
	size_t capacityBytes() const;
	id_t size() const { return current; }
	size_t sizeBytes() const;

	void setHeader(const ServerSnapshotHeader& header);
	void add(const SnapshotObject& object);
	void reset() { current = 0; }
	void invalidate();
	void dump();

	char* getData() const { return buffer; }
	const ServerSnapshotHeader* getHeader() const { return (ServerSnapshotHeader*)buffer; }
	SnapshotObject* get(id_t index) const;

	inline SnapshotObject* operator[](id_t index) const { return get(index); }
};

size_t SnapshotBuffer::capacityBytes() const {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * _capacity;
}

size_t SnapshotBuffer::sizeBytes() const {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * current;
}

SnapshotBuffer::SnapshotBuffer(id_t capacity) : _capacity(capacity), current(0) {
	buffer = new char[capacityBytes()];
	objects = getFirstObject();
}

SnapshotBuffer::SnapshotBuffer(char* _buffer) : buffer(_buffer) {
	current = getHeader()->getSnapshotSize();
	_capacity = current;
	objects = getFirstObject();
}

SnapshotBuffer::~SnapshotBuffer() {
	if (buffer) delete[] buffer;
}

void SnapshotBuffer::setHeader(const ServerSnapshotHeader& header) {
	memcpy(buffer, &header, sizeof(header));
}

void SnapshotBuffer::add(const SnapshotObject& object) {
	if (current >= _capacity) throw std::runtime_error("SnapshotBuffer index out of bounds!");
	objects[current] = object;
	current++;
}

void SnapshotBuffer::invalidate() {
	buffer = nullptr;
	objects = nullptr;
}

void SnapshotBuffer::dump() {
	std::println("Buffer size: {}", size());
	for (id_t i = 0; i < size(); i++) {
		id_t id = (objects + i)->id;
		std::println("ID: {0}, {1} - {2}", id, NULL_CLIENT, (id == NULL_CLIENT));
	}
}

SnapshotObject* SnapshotBuffer::get(id_t index) const {
	return objects + index;
}