#pragma once

#include <stdint.h>
#include <cstdlib>
#include <memory.h>

class OMStream {
private:
	uint8_t* buffer;
	size_t head;
	size_t capacity;

public:
	OMStream();

	void resize(size_t size);
};

OMStream::OMStream() : head(0), capacity(32), buffer(nullptr) {
	resize(32);
}

void OMStream::resize(size_t size) {
	capacity = size;
	buffer = (uint8_t*)realloc(buffer, capacity);
}

class IMStream {
private:
public:
};