#include <iostream>
#include <print>
#include <array>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include "stdint.h"

#include <enet/enet.h>
#include <glm/glm.hpp>
#include <entt.hpp>

#include "network.h"
#include "packet_types.h"
#include "components.h"

#define SERVER_SIZE		10

template <id_t Size> class ClientStorage;
using DefaultClientStorage = ClientStorage<SERVER_SIZE>;

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
	id_t getLastFree() const { return freeIndicies.back(); }

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
	id_t index = freeIndicies.back();

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

struct NetworkContext {
	DefaultClientStorage clients;
	entt::registry world;
};

entt::entity createPlayer(entt::registry& world, const glm::vec2& position, id_t id) {
	entt::entity player = world.create();
	world.emplace<CompNetworkId>(player).id = id;
	world.emplace<CompCharacter>(player).position = position;
	return player;
}

void clientConnected(Network* network, NetworkPeer peer, void* data) {
	NetworkContext* context = (NetworkContext*)data;
	const ClientData& newClient = context->clients.add(ClientData(peer));
	peer.setData((void*)&newClient);

	std::println("Client connected! Server size: {}", context->clients.size());

	entt::registry& world = context->world;
	entt::entity player = createPlayer(world, { 0.0f, 0.0f }, newClient.getId());
	context->clients[newClient.getId()].setPlayer(player);
	
	ServerHelloPacket serverHello(newClient.getId());
	network->sendTo(peer, serverHello);
}

void clientDisconnected(Network* network, NetworkPeer peer, void* data) {
	NetworkContext* context = (NetworkContext*)data;
	ClientData* clientData = (ClientData*)peer.getData();

	std::println("Going to remove {0}", clientData->getId());
	context->clients.remove(((ClientData*)peer.getData())->getId());
	peer.invalidate();

	id_t size = context->clients.size();
	std::println("Client disconnected! Server size: {}", size);
}

int main() {
	Network network;
	NetworkContext context;
	
	network.setContext(&context);
	network.onConnectReceived(clientConnected);
	network.onDisconnectReceived(clientDisconnected);

	try {
		network.host("127.0.0.1", 27015);
		std::println("Server hosted!");
	}
	catch (NetworkException exc) {
		std::println("{0}", exc.what());
	}

	bool running = true;
	int fpsLimit = 70;
	float frameDuration = 1000.0f / fpsLimit;

	const float tps = 30.0f;
	const float tickTime = 1.0f / tps;
	float accumulator = 0.0f;
	timePoint beg = std::chrono::steady_clock::now();
	timePoint end;

	while (running) {
		end = std::chrono::steady_clock::now();
		std::chrono::duration<float> elapsedTime = end - beg;
		beg = end;

		network.poll();

		accumulator += elapsedTime.count();
		while (accumulator >= tickTime) {
			accumulator -= tickTime;
			//std::println("Do something...");
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}