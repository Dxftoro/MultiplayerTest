#include <iostream>
#include <print>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include "stdint.h"

#include <enet/enet.h>
#include <glm/glm.hpp>
#include <entt.hpp>

#include "network.h"
#include "packet_types.h"
#include "components.h"
#include "client_storage.h"

#define SERVER_SIZE		16

using DefaultClientStorage = ClientStorage<SERVER_SIZE>;

struct NetworkContext {
	DefaultClientStorage clients;
	SnapshotBuffer snapshotBuffer;
	entt::registry world;

	NetworkContext() : snapshotBuffer(SERVER_SIZE) {}
};

float frand(const float a, const float b) {
	return a + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (b - a));
}

entt::entity createPlayer(entt::registry& world, const glm::vec2& position, id_t id) {
	entt::entity player = world.create();
	world.emplace<CompNetworkId>(player).id = id;
	world.emplace<CompCharacter>(player).position = position;
	return player;
}

void removePlayer(entt::registry& world, entt::entity player) {
	world.erase<CompCharacter>(player);
	world.erase<CompNetworkId>(player);
	std::println("Player removed.");
}

void clientConnected(Network* network, NetworkPeer peer, void* data) {
	NetworkContext* context = (NetworkContext*)data;
	const ClientData& newClient = context->clients.add(ClientData(peer));
	peer.setData((void*)&newClient);

	std::println("Client connected! Server size: {}", context->clients.size());

	entt::registry& world = context->world;
	entt::entity player = createPlayer(world, { frand(1.0f, 300.0f), frand(1.0f, 300.0f) }, newClient.getId());
	context->clients[newClient.getId()].setPlayer(player);
	
	ServerHelloPacket serverHello(newClient.getId(), SERVER_SIZE);
	network->sendTo(peer, serverHello);
}

void clientDisconnected(Network* network, NetworkPeer peer, void* data) {
	NetworkContext* context = (NetworkContext*)data;
	ClientData* clientData = (ClientData*)peer.getData();
	id_t clientId = clientData->getId();

	removePlayer(context->world, clientData->getPlayer());
	context->clients.remove(clientId);
	peer.invalidate();

	ClientDisconnectedPacket packet(clientId);
	network->broadcast(packet);

	id_t size = context->clients.size();
	std::println("Client [{0}] disconnected! Server size: {1}", clientId, size);
}

void snapshotMergeSystem(NetworkContext& context) {
	context.world.view<CompNetworkId, CompCharacter>()
	.each([&context](entt::entity, CompNetworkId& networkId, CompCharacter& character) {
		//if (!character.dirty) return;
		//character.dirty = false;

		SnapshotObject object(networkId.id, character.position, (CompCharacter::State)character.state);
		context.snapshotBuffer.add(object);
		//std::println("Added object with ID: {}",
		//	context.snapshotBuffer[context.snapshotBuffer.size() - 1]->id);
	});

	ServerSnapshotHeader snapshotHeader(context.snapshotBuffer.size());
	context.snapshotBuffer.setHeader(snapshotHeader);
}

void sendSnapshot(Network& network, NetworkContext& context) {
	network.broadcast(context.snapshotBuffer.getData(), context.snapshotBuffer.sizeBytes());
}

void moveCharacterByWishDir(entt::registry& world, entt::entity player, WishDir wishDir, float tickTime) {
	CompCharacter& character = world.get<CompCharacter>(player);
	glm::vec2 velocity = glm::vec2((float)wishDir.x, (float)wishDir.y) * CompCharacter::speed;
	character.position += velocity;
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

	srand(std::time(0));
	bool running = true;
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
		network.getMessageBuffer()->each<[](NetworkMessage& message, void* data) {
			NetworkContext* context = (NetworkContext*)data;
			UnknownPacket* packet = message.getPacket().data<UnknownPacket>();

			switch (packet->getType()) {
			case PacketType::CLIENT_MOVEMENT: {
				ClientMovementPacket* movementPacket = (ClientMovementPacket*)packet;
				ClientData* clientData = (ClientData*)message.getSender().getData();
				moveCharacterByWishDir(
					context->world,
					clientData->getPlayer(),
					movementPacket->getWishDir(),
					1 / 30.0f);
				break;
			}
			default:
				std::println("Received an unexpected packet type!");
				break;
			}
		}>();

		accumulator += elapsedTime.count();
		while (accumulator >= tickTime) {
			accumulator -= tickTime;

			snapshotMergeSystem(context);
			//std::println("Snapshot size: {}", context.snapshotBuffer.size());

			if (context.snapshotBuffer.size()) {
				//std::println("Something happened! Sending snapshot. Size: {}", 
				//	context.snapshotBuffer.size());
				sendSnapshot(network, context);
				context.snapshotBuffer.reset();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}