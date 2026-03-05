#pragma once
#include <stdint.h>
#include <type_traits>

enum class PacketType : uint8_t {
	UNKNOWN,
	SERVER_HELLO
};

template <PacketType Type>
class Packet {
protected:
	PacketType type;

public:
	Packet() : type(Type) {}
	PacketType getType() const { return type; }
	static constexpr PacketType sGetType() { return Type; }
};

template <typename T>
concept PacketOne = std::is_base_of_v<Packet<T::sGetType()>, T>;