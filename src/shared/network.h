#pragma once
#include <string>
#include <array>
#include <enet/enet.h>

class NetworkException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

template <typename ENetT>
class NetworkInstance {
protected:
	ENetT* instance;

public:
	NetworkInstance() : instance(nullptr) {}

	ENetT* get() const { return instance; }
	void invalidate() { this->instance = nullptr; }
	bool isValid() const { return (this->instance != nullptr); }
};

class NetworkPeer : public NetworkInstance<ENetPeer> {
public:
	NetworkPeer(ENetPeer* peer) { this->instance = peer; }

	void setData(void* data) { this->instance->data = data; }
	void* getData() const { return this->instance->data; }
};

class NetworkHost : public NetworkInstance<ENetHost> {
public:
	NetworkHost(ENetHost* host) { this->instance = host; }
};

class NetworkPacket : public NetworkInstance<ENetPacket> {
public:
	NetworkPacket(ENetPacket* packet) { this->instance = packet; }

	void* getData() const { return this->instance->data; }
	size_t getDataSize() const { return this->instance->dataLength; }
};

class NetworkMessage {
private:
	NetworkPeer sender;
	NetworkPacket packet;
	
public:
	NetworkMessage() : sender(nullptr), packet(nullptr) {}
	NetworkMessage(NetworkPeer sender, NetworkPacket packet);
	~NetworkMessage();

	void release();

	NetworkPeer getSender() const { return sender; }
	NetworkPacket getPacket() const { return packet; }
	bool isReleased() const { return !sender.get() && !packet.get(); }
};

NetworkMessage::NetworkMessage(NetworkPeer _sender, NetworkPacket _packet)
:	sender(_sender), packet(_packet) {}

NetworkMessage::~NetworkMessage() { /* Just an empty destructor */ }

void NetworkMessage::release() {
	enet_packet_destroy(packet.get());
	packet.invalidate();
	sender.invalidate();
}

class NetworkMessageBuffer {
	friend class Network;
public:
	static constexpr size_t BUFFER_SIZE = 512;

private:
	size_t _current;
	void* context;
	std::array<NetworkMessage, BUFFER_SIZE> messages;
	
public:
	NetworkMessageBuffer() : _current(0), context(nullptr) {}

	void next() { _current++; }
	void reset() { _current = 0; }
	void push(const NetworkMessage& message) { messages[_current] = message; next(); }

	NetworkMessage& get(size_t index) { return messages[index]; }
	void* getContext() const { return context; }
	size_t size() const { return _current; }
	inline NetworkMessage& operator[](size_t index) { return get(index); }

	template<auto Func> void each();
};

template<auto Func>
void NetworkMessageBuffer::each() {
	size_t i = 0;
	for (i = 0; i < size(); i++) {
		Func(messages[i], context);
		std::println("Releasing {}", i);
		messages[i].release();
	}
}

class Network {
private:
	NetworkPeer _peer;
	NetworkHost _host;
	NetworkMessageBuffer* messageBuffer;
	bool hosting, connected;

	using ConnectionCallback = void (*)(Network*, NetworkPeer, void* context);
	ConnectionCallback onConnect, onDisconnect;

	Network(const Network&) = delete;
	Network& operator=(const Network&) = delete;

	ENetAddress setupAddress(const std::string& ip, unsigned int port);

public:
	Network();
	~Network();

	void host(const std::string& ip, unsigned int port);
	void connect(const std::string& ip, int port);
	void broadcast(const char* data, size_t size);
	void send(const char* data, size_t size);
	void sendTo(NetworkPeer peer, const char* data, size_t size);
	void disconnect();
	void poll();

	void setContext(void* context) { messageBuffer->context = context; }
	void onConnectReceived(ConnectionCallback onConnect) { this->onConnect = onConnect; }
	void onDisconnectReceived(ConnectionCallback onDisconnect) { this->onDisconnect = onDisconnect; }

	bool isHosting() const { return hosting; }
	bool isConnected() const { return connected; }
	NetworkMessageBuffer* getMessageBuffer() const { return messageBuffer; }
};

Network::Network()
	:	_peer(nullptr), _host(nullptr),
		onConnect(nullptr), onDisconnect(nullptr),
		hosting(false), connected(false) {

	enet_initialize();
	messageBuffer = new NetworkMessageBuffer();
}

Network::~Network() {
	disconnect();
	if (_host.get()) enet_host_destroy(_host.get());
	enet_deinitialize();
	delete messageBuffer;
}

ENetAddress Network::setupAddress(const std::string& ip, unsigned int port) {
	ENetAddress address;
	enet_address_set_host(&address, ip.c_str());
	address.port = port;
	return address;
}

void Network::host(const std::string& ip, unsigned int port) {
	ENetAddress address = setupAddress(ip, port);

	ENetHost* server = enet_host_create(&address, 32, 1, 0, 0);
	if (!server) {
		throw NetworkException("Host creation error!");
	}

	_host = NetworkHost(server);
	hosting = true;
	connected = true;
}

void Network::connect(const std::string& ip, int port) {
	ENetAddress address = setupAddress(ip, port);
	
	ENetHost* client = enet_host_create(nullptr, 1, 1, 0, 0);
	if (!client) {
		throw NetworkException("Host creation error!");
	}

	ENetPeer* peer = enet_host_connect(client, &address, 1, 0);
	if (!peer) {
		throw NetworkException("Peer creation error!");
	}

	_host = NetworkHost(client);
	_peer = NetworkPeer(peer);

	ENetEvent event;
	int serviceStatus = enet_host_service(_host.get(), &event, 5000);

	if (serviceStatus <= 0 || event.type != ENET_EVENT_TYPE_CONNECT) {
		throw NetworkException(std::format("Connection to {0}:{1} failed!", ip, port));
	}

	connected = true;
}

void Network::broadcast(const char* data, size_t size) {
	ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(_host.get(), 0, packet);
}

void Network::send(const char* data, size_t size) {
	ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(_peer.get(), 0, packet);
}

void Network::sendTo(NetworkPeer peer, const char* data, size_t size) {
	ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer.get(), 0, packet);
}

void Network::disconnect() {
	if (!connected || hosting) return;
	ENetEvent event;

	enet_peer_disconnect(_peer.get(), 0);

	while (enet_host_service(_host.get(), &event, 1000) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			std::println("Disconnection succeeded!");
			break;
		}
	}

	connected = false;
}

void Network::poll() {
	if (!connected) return;
	//std::println("Doing poll...");

	messageBuffer->reset();
	ENetEvent event;

	if (!_host.get()) {
		std::println("ERROR: _host is NULL!");
		return;
	}

	while (enet_host_service(_host.get(), &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			if (onConnect) onConnect(this, event.peer, messageBuffer->getContext());
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			messageBuffer->push(NetworkMessage(event.peer, event.packet));
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			if (onDisconnect) onDisconnect(this, event.peer, messageBuffer->getContext());
			break;
		}
	}
}