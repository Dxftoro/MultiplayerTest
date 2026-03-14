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
#include "client_storage.h"
#include "components.h"
#include "glsl_program.h"

#define S_WIDTH			480
#define S_HEIGHT		480
#define SERVER_SIZE		10
#define COLOR_WHITE		{1.0f, 1.0f, 1.0f}
#define COLOR_RED		{1.0f, 0.0f, 0.0f}

using DefaultSnapshotBuffer = SnapshotBuffer<SERVER_SIZE>;
using DefaultClientStorage = ClientStorage<SERVER_SIZE>;

float verticies[] = {
	-1.0f, 1.0f,
	-1.0f, -1.0f,
	1.0f, 1.0f,
	1.0f, -1.0f
};

entt::entity spawnCharacter(entt::registry& world, const glm::vec2& position) {
	entt::entity entity = world.create();
	world.emplace<CompCharacter>(entity).position = position;
	return entity;
}

struct NetworkContext {
	Network* network;
	id_t localId;
	entt::registry world;
	std::vector<ClientData> clients;
};

class CharacterDrawSystem {
private:
	GLFWwindow* window;
	vray::GlslProgram program;
	vray::GlslUniform uProjectionMatrix, uPosition;
	glm::mat4 projection;
	GLuint vao, vbo;
	entt::registry& world;

	using CharacterView = decltype(world.view<CompCharacter>());
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
		program.compileShader("generic.vert", vray::ShaderType::VERTEX);
		program.compileShader("generic.frag", vray::ShaderType::FRAGMENT);
		program.link();
		program.validate();
	}
	catch (vray::GlslException exc) {
		std::println("{}", exc.what());
		return;
	}

	uProjectionMatrix = program.getUniform("uProjectionMatrix");
	uPosition = program.getUniform("uPosition");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	characters = world.view<CompCharacter>();
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
	for (auto [entity, character] : characters.each()) {
		program.setUniform(uPosition, glm::vec3(character.position, 0.0f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glfwSwapBuffers(window);
	glfwPollEvents();
}

void snapshotMergeSystem(DefaultSnapshotBuffer& buffer, NetworkContext* context) {
	for (id_t i = 0; i < buffer.size(); i++) {
		id_t id = buffer[i]->id;

		if (context->clients[id].getId() == NULL_CLIENT) {
			context->clients[id].setId(id);
			entt::entity player = spawnCharacter(context->world, buffer[i]->position);
			context->clients[id].setPlayer(player);
		}
		else {
			entt::entity player = context->clients[id].getPlayer();
			CompCharacter& character = context->world.get<CompCharacter>(player);
			character.position = buffer[i]->position;
		}
	}
}

int main() {
	Network network;
	NetworkMessageBuffer* messages = network.getMessageBuffer();
	
	NetworkContext context = {
		.network	= &network,
		.localId	= NULL_CLIENT
	};
	network.setContext(&context);

	if (!glfwInit()) { return -2; }

	while (!network.isConnected()) {
		try {
			std::println("Trying to connect to the server...");
			network.connect("127.0.0.1", 27015);
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

	while (!glfwWindowShouldClose(window)) {
		end = std::chrono::steady_clock::now();
		timePoint frameEnd = end + std::chrono::milliseconds(1000 / fpsLimit);
		std::chrono::duration<float> elapsedTime = end - beg;
		beg = end;

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
				break;
			}
			case PacketType::SERVER_SNAPSHOT_HEADER: {
				DefaultSnapshotBuffer buffer((char*)message.getPacket().getData());
				snapshotMergeSystem(buffer, context);
				buffer.invalidate();
				break;
			}
			default:
				std::println("Unexpected packet type!");
				break;
			}
		}>();

		characterDrawSystem.update();

		std::this_thread::sleep_until(frameEnd);
	}

	network.disconnect();

	glfwTerminate();
	enet_deinitialize();
	return 0;
}