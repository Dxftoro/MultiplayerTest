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
	bool copy;

	SnapshotObject* getFirstObject() const { return (SnapshotObject*)(buffer + sizeof(ServerSnapshotHeader)); }

public:
	SnapshotBuffer(id_t capacity);
	SnapshotBuffer(char* buffer);
	~SnapshotBuffer();

	SnapshotBuffer(const SnapshotBuffer&);
	SnapshotBuffer(SnapshotBuffer&&) noexcept;

	static size_t sCapacityBytes(id_t capacity);
	static size_t sSizeBytes(id_t size);

	id_t capacity() const { return _capacity; }
	size_t capacityBytes() const { return sCapacityBytes(_capacity); }
	id_t size() const { return current; }
	size_t sizeBytes() const { return sSizeBytes(current); }

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

size_t SnapshotBuffer::sCapacityBytes(id_t capacity) {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * capacity;
}

size_t SnapshotBuffer::sSizeBytes(id_t size) {
	return sizeof(ServerSnapshotHeader) + sizeof(SnapshotObject) * size;
}

SnapshotBuffer::SnapshotBuffer(id_t capacity) : _capacity(capacity), current(0), copy(false) {
	buffer = new char[capacityBytes()];
	objects = getFirstObject();
}

SnapshotBuffer::SnapshotBuffer(char* _buffer) : buffer(_buffer), copy(true) {
	current = getHeader()->getSnapshotSize();
	_capacity = current;
	objects = getFirstObject();
}

SnapshotBuffer::~SnapshotBuffer() {
	if (buffer && !copy) delete[] buffer;
}

SnapshotBuffer::SnapshotBuffer(const SnapshotBuffer& buffer) : copy(true) {
	//std::println("Buffer copy!");
	this->buffer = buffer.buffer;
	this->objects = buffer.objects;
	this->current = buffer.current;
	this->_capacity = buffer._capacity;
}

SnapshotBuffer::SnapshotBuffer(SnapshotBuffer&& buffer) noexcept : copy(false) {
	//std::println("Buffer move!");
	this->buffer = buffer.buffer;
	this->objects = buffer.objects;
	this->current = buffer.current;
	this->_capacity = buffer._capacity;
	buffer.invalidate();
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