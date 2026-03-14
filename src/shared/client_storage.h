#pragma once

#include <array>
#include <queue>
#include <entt.hpp>

#include "network.h"

class ClientData {
private:
	id_t id;
	NetworkPeer peer;
	entt::entity player;

public:
	ClientData() : id(NULL_CLIENT), peer(nullptr), player(entt::null) {}
	explicit ClientData(NetworkPeer _peer) : id(NULL_CLIENT), peer(_peer), player(entt::null) {}

	void setId(id_t id) { this->id = id; }
	void setPlayer(entt::entity player) { this->player = player; }

	NetworkPeer getPeer() const { return peer; }
	id_t getId() const { return id; }
	entt::entity getPlayer() const { return player; }

	bool isNull() const { return getId() == NULL_CLIENT; }
};

template <id_t Size>
class ClientStorage {
private:
	std::array<ClientData, Size> clients;
	std::queue<id_t> freeIndicies;

public:
	ClientStorage();

	const ClientData& add(const ClientData& clientData);
	ClientData& get(id_t id);
	void remove(id_t id);

	constexpr id_t capacity() const { return Size; }
	id_t size() const { return Size - freeIndicies.size(); }
	id_t getLastFree() const { return freeIndicies.front(); }

	inline ClientData& operator[](id_t id) { return get(id); }
};

template <id_t Size>
ClientStorage<Size>::ClientStorage() {
	for (id_t i = 0; i < Size; i++) {
		freeIndicies.push(i);
	}
}

template <id_t Size>
const ClientData& ClientStorage<Size>::add(const ClientData& clientData) {
	id_t index = freeIndicies.front();
	std::println("New client id: {}", index);

	clients[index] = clientData;
	clients[index].setId(index);
	freeIndicies.pop();

	return clients[index];
}

template <id_t Size>
ClientData& ClientStorage<Size>::get(id_t id) {
	return clients[id];
}

template <id_t Size>
void ClientStorage<Size>::remove(id_t id) {
	clients[id] = ClientData();
	freeIndicies.push(id);
}