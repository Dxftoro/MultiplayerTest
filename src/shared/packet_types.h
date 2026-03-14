#pragma once

#include "maindef.h"
#include "packet_base.h"

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

public:
	ServerSnapshotHeader() : snapshotSize(0) {}
	ServerSnapshotHeader(id_t _snapshotSize) : snapshotSize(_snapshotSize) {}

	id_t getSnapshotSize() const { return snapshotSize; }
};