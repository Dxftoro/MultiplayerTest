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
#include "components.h"
#include "glsl_program.h"

#define S_WIDTH 480
#define S_HEIGHT 480

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
};

int main() {
	Network network;
	NetworkMessageBuffer* messageBuffer = network.getMessageBuffer();
	
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

	for (int i = 0; i < 50; i++) network.send("Get out of there, jabroni outfit!", 25);

	GLFWwindow* window = glfwCreateWindow(S_WIDTH, S_HEIGHT, "Client", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -3;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGL()) { return -4; }

	vray::GlslProgram program;

	try {
		program.compileShader("generic.vert", vray::ShaderType::VERTEX);
		program.compileShader("generic.frag", vray::ShaderType::FRAGMENT);
		program.link();
		program.validate();
	}
	catch (vray::GlslException exc) {
		std::println("{}", exc.what());
		return -1;
	}

	vray::GlslUniform uProjectionMatrix = program.getUniform("uProjectionMatrix");
	vray::GlslUniform uPosition = program.getUniform("uPosition");

	glm::mat4 projection = glm::ortho(0.0f, (float)S_WIDTH, (float)S_HEIGHT, 0.0f, -1.0f, 1.0f);

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	timePoint beg = std::chrono::steady_clock::now();
	timePoint end;

	int fpsLimit = 70;
	float frameDuration = 1000.0f / fpsLimit;

	auto characterView = context.world.view<CompCharacter>();

	while (!glfwWindowShouldClose(window)) {
		end = std::chrono::steady_clock::now();
		timePoint frameEnd = end + std::chrono::milliseconds(1000 / fpsLimit);
		std::chrono::duration<float> elapsedTime = end - beg;
		beg = end;

		network.poll();
		messageBuffer->each<[](NetworkMessage& message, void* data) {
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
				break;
			}
			default:
				std::println("Unexpected packet type!");
				break;
			}

		}>();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		program.use();
		program.setUniform(uProjectionMatrix, projection);

		glBindVertexArray(vao);
		for (auto [entity, character] : characterView.each()) {
			program.setUniform(uPosition, glm::vec3(character.position, 0.0f));
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		std::this_thread::sleep_until(frameEnd);
	}

	network.disconnect();

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	glfwTerminate();
	enet_deinitialize();
	return 0;
}