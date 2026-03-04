#pragma once
#include <stdint.h>
#include <type_traits>

enum class PacketType : uint8_t {
	SERVER_HELLO,
	TEST
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
struct is_packet : std::false_type {};

template <PacketType Type>
struct is_packet<Packet<Type>> : std::true_type {};

template <typename T>
concept PacketOne = is_packet<T>::value;