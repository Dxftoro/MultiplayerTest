#include <print>
#include <iostream>
#include <chrono>
#include <thread>

#include <enet/enet.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <entt.hpp>

#include "maindef.h"
#include "network.h"
#include "packet_types.h"
#include "snapshot_buffer.h"
#include "snapshot_pool.h"
#include "client_storage.h"
#include "components.h"
#include "glsl_program.h"

#define S_WIDTH			480
#define S_HEIGHT		480
#define COLOR_WHITE		glm::vec3(1.0f, 1.0f, 1.0f)
#define COLOR_RED		glm::vec3(1.0f, 0.0f, 0.0f)

using DefaultSnapshotPool = SnapshotPool<16>;

float verticies[] = {
	-1.0f, 1.0f,
	-1.0f, -1.0f,
	1.0f, 1.0f,
	1.0f, -1.0f
};

entt::entity spawnCharacter(entt::registry& world, const glm::vec2& position, const glm::vec3& color) {
	std::println("Spawning character");
	entt::entity entity = world.create();
	world.emplace<CompCharacter>(entity).position = position;
	world.emplace<CompColor>(entity).color = color;
	return entity;
}

void removeCharacter(entt::registry& world, entt::entity player) {
	world.erase<CompCharacter>(player);
	world.erase<CompColor>(player);
}

struct NetworkContext {
	Network* network;
	id_t localId;
	entt::registry world;
	std::vector<ClientData> clients;
	DefaultSnapshotPool snapshotPool;
};

class CharacterDrawSystem {
private:
	GLFWwindow* window;
	vray::GlslProgram program;
	vray::GlslUniform uProjectionMatrix, uPosition, uColor;
	glm::mat4 projection;
	GLuint vao, vbo;
	entt::registry& world;

	using CharacterView = decltype(world.view<CompCharacter, CompColor>());
	CharacterView characters;
	bool initialized;

private:
	CharacterDrawSystem(const CharacterDrawSystem&) = delete;
	CharacterDrawSystem(CharacterDrawSystem&&) = delete;
	CharacterDrawSystem& operator=(const CharacterDrawSystem&) = delete;

public:
	CharacterDrawSystem(GLFWwindow* window, entt::registry& world);
	~CharacterDrawSystem();

	glm::mat4& getProjection() { return projection; }
	bool isInitialized() const { return initialized; }

	void update();
};

CharacterDrawSystem::CharacterDrawSystem(GLFWwindow* _window, entt::registry& _world) 
:	window(_window), world(_world), vao(0), vbo(0), initialized(false),
	projection(glm::ortho(0.0f, (float)S_WIDTH, (float)S_HEIGHT, 0.0f, -1.0f, 1.0f)) {

	if (!gladLoadGL()) { return; }

	try {
		program.compileShader("shaders/generic.vert", vray::ShaderType::VERTEX);
		program.compileShader("shaders/generic.frag", vray::ShaderType::FRAGMENT);
		program.link();
		program.validate();

		uProjectionMatrix = program.getUniform("uProjectionMatrix");
		uPosition = program.getUniform("uPosition");
		uColor = program.getUniform("uColor");
	}
	catch (vray::GlslException exc) {
		std::println("{0} [{1}]", exc.what(), glGetError());
		return;
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	characters = world.view<CompCharacter, CompColor>();
	initialized = true;
}

CharacterDrawSystem::~CharacterDrawSystem() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

void CharacterDrawSystem::update() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	program.use();
	program.setUniform(uProjectionMatrix, projection);

	glBindVertexArray(vao);
	for (auto [entity, character, color] : characters.each()) {
		program.setUniform(uPosition, glm::vec3(character.position, 0.0f));
		program.setUniform(uColor, color.color);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glfwSwapBuffers(window);
	glfwPollEvents();
}

void snapshotMergeSystem(SnapshotBuffer& buffer, NetworkContext* context) {
	for (id_t i = 0; i < buffer.size(); i++) {
		id_t id = buffer[i]->id;

		//if (id >= context->clients.size()) continue;

		if (context->clients[id].getId() == NULL_CLIENT) {
			context->clients[id].setId(id);
			
			entt::entity player = spawnCharacter(
				context->world,
				buffer[i]->position,
				(id == context->localId ? COLOR_RED : COLOR_WHITE));
			
			context->clients[id].setPlayer(player);
		}
		else {
			entt::entity player = context->clients[id].getPlayer();
			CompCharacter& character = context->world.get<CompCharacter>(player);
			character.position = buffer[i]->position;
		}
	}
}

struct Test {
	int a, b;
	float t;

	Test(int _a, int _b, float _t) : a(_a), b(_b), t(_t) {}
	~Test() { std::println("Test destructor"); }

	void dump() { std::println("{0}, {1}, {2}", a, b, t); }
};

std::optional<Test> testOpt() {
	std::optional<Test> test(std::in_place, 1, 2, 3.0f);
	return test;
}

void interpolateSnapshots(DefaultSnapshotPool& snapshotPool, NetworkContext* context) {
	std::optional<SnapshotPair> pair = snapshotPool.getInterpolationPair(utime::now());
	if (!pair) return;

	snapshotMergeSystem(pair->a, context);
	if (pair->a.getData() == pair->b.getData()) return;

	for (id_t i = 0; i < pair->b.size(); i++) {
		id_t id = pair->b[i]->id;

		//if (id >= context->clients.size()) continue;

		if (context->clients[id].getId() == NULL_CLIENT) {
			context->clients[id].setId(id);

			entt::entity player = spawnCharacter(
				context->world,
				pair->b[i]->position,
				(id == context->localId ? COLOR_RED : COLOR_WHITE));

			context->clients[id].setPlayer(player);
		}
		else {
			entt::entity player = context->clients[id].getPlayer();
			CompCharacter& character = context->world.get<CompCharacter>(player);
			character.position = glm::mix(character.position, pair->b[i]->position, pair->t);
		}
	}
}

void inputSystem(GLFWwindow* window, Network& network) {
	int A = glfwGetKey(window, GLFW_KEY_A);
	int D = glfwGetKey(window, GLFW_KEY_D);
	int W = glfwGetKey(window, GLFW_KEY_W);
	int S = glfwGetKey(window, GLFW_KEY_S);
	WishDir wishDir((char)(-A + D), (char)(-W + S));

	ClientMovementPacket movementPacket(wishDir);
	network.send(movementPacket);
}

void countFps(GLFWwindow* window, float& elapsedTime) {
	while (!glfwWindowShouldClose(window)) {
		//std::println("FPS: {}", (1.0f / elapsedTime));
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
}

int main() {
	Network network;
	NetworkMessageBuffer* messages = network.getMessageBuffer();
	
	NetworkContext context = {
		.network		= &network,
		.localId		= NULL_CLIENT
	};
	network.setContext(&context);

	if (!glfwInit()) { return -2; }

	while (!network.isConnected()) {
		try {
			std::println("Trying to connect to the server...");
			network.connect("26.60.242.39", 27015);
			std::println("Connection succeded!");
		}
		catch (NetworkException exc) {
			std::println("{}", exc.what());
		}
	}

	GLFWwindow* window = glfwCreateWindow(S_WIDTH, S_HEIGHT, "Client", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -3;
	}

	glfwMakeContextCurrent(window);

	CharacterDrawSystem characterDrawSystem(window, context.world);
	if (!characterDrawSystem.isInitialized()) { return -4; }

	timePoint beg = std::chrono::steady_clock::now();
	timePoint end;

	int fpsLimit = 70;
	float frameDuration = 1000.0f / fpsLimit;
	float elapsedTime = 1.0f;

	std::thread fpsCounter(countFps, window, std::ref(elapsedTime));

	while (!glfwWindowShouldClose(window)) {
		end = std::chrono::steady_clock::now();
		timePoint frameEnd = end + std::chrono::milliseconds(1000 / fpsLimit);
		std::chrono::duration<float> diff = end - beg;
		elapsedTime = diff.count();
		beg = end;

		inputSystem(window, network);
		network.poll();

		messages->each<[](NetworkMessage& message, void* data) {
			NetworkContext* context = (NetworkContext*)data;
			UnknownPacket* packet = message.getPacket().data<UnknownPacket>();

			switch (packet->getType()) {
			case PacketType::SERVER_HELLO: {
				ServerHelloPacket* hello = (ServerHelloPacket*)packet;
				
				std::println(
					"Received server hello ({0}), id: {1}",
					(uint8_t)hello->getType(),
					hello->getClientId());
				
				context->localId = hello->getClientId();
				context->clients = std::vector<ClientData>(hello->getServerSize());
				context->snapshotPool.init(hello->getServerSize());
				break;
			}
			case PacketType::CLIENT_DISCONNECTED: {
				ClientDisconnectedPacket* disconnected = (ClientDisconnectedPacket*)packet;

				std::println("Received client disconnected. ID: {}", disconnected->getClientId());

				entt::entity player = context->clients[disconnected->getClientId()].getPlayer();
				removeCharacter(context->world, player);
				context->clients[disconnected->getClientId()].setId(NULL_CLIENT);
				break;
			}
			case PacketType::SERVER_SNAPSHOT_HEADER: {
				context->snapshotPool.push((char*)message.getPacket().getData());

				//SnapshotBuffer buffer((char*)message.getPacket().getData());
				//snapshotMergeSystem(buffer, context);
				break;
			}
			default:
				std::println("Unexpected packet type!");
				break;
			}
		}>();

		interpolateSnapshots(context.snapshotPool, &context);
		characterDrawSystem.update();

		std::this_thread::sleep_until(frameEnd);
	}

	network.disconnect();
	fpsCounter.join();

	glfwTerminate();
	enet_deinitialize();
	return 0;
}