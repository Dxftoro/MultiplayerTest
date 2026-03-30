#pragma once

#include "maindef.h"
#include "packet_base.h"

#include <glm/glm.hpp>

class UnknownPacket : public Packet<PacketType::UNKNOWN> {
public:
	using Packet<PacketType::UNKNOWN>::Packet;
};

class ServerHelloPacket : public Packet<PacketType::SERVER_HELLO> {
private:
	id_t clientId;
	id_t serverSize;

public:
	ServerHelloPacket(id_t _clientId, id_t _serverSize)
		: clientId(_clientId), serverSize(_serverSize) {}
	ServerHelloPacket() : clientId(NULL_CLIENT), serverSize(1) {}

	id_t getClientId() const { return clientId; }
	id_t getServerSize() const { return serverSize; }
};

class ServerSnapshotHeader : public Packet<PacketType::SERVER_SNAPSHOT_HEADER> {
private:
	id_t snapshotSize;
	time_t timestamp;

public:
	ServerSnapshotHeader() : snapshotSize(0), timestamp(time::now()) {}
	ServerSnapshotHeader(id_t _snapshotSize) : snapshotSize(_snapshotSize), timestamp(time::now()) {}

	id_t getSnapshotSize() const { return snapshotSize; }
	time_t getTimestamp() const { return timestamp; }
};

using WishDir = glm::vec<2, char>;

class ClientMovementPacket : public Packet<PacketType::CLIENT_MOVEMENT> {
private:
	WishDir wishDir;

public:
	ClientMovementPacket() : wishDir(0) {}
	ClientMovementPacket(WishDir _wishDir) : wishDir(_wishDir) {}

	WishDir getWishDir() const { return wishDir; }
};

class ClientDisconnectedPacket : public Packet<PacketType::CLIENT_DISCONNECTED> {
private:
	id_t clientId;

public:
	ClientDisconnectedPacket() : clientId(NULL_CLIENT) {}
	ClientDisconnectedPacket(id_t _clientId) : clientId(_clientId) {}

	id_t getClientId() const { return clientId; }
};