#pragma once
#include <glm/glm.hpp>
#include <entt.hpp>

#include "maindef.h"
#include "network.h"

struct CompCharacter {
	static constexpr float speed = 1.0f;

	glm::vec2 position, velocity, direction;
	enum State { IDLE, MOVING } state;
};

struct CompNetworkId {
	id_t id = NULL_CLIENT;
	bool isLocal = false;
};

void characterMovementSystem(entt::registry& world, float deltaTime) {
	auto characterView = world.view<CompCharacter>();
	static float acceleration = 1.1f;

	characterView.each([deltaTime](entt::entity entity, CompCharacter& character) {
		if (character.state == CompCharacter::State::MOVING) {
			character.velocity += (character.direction * CompCharacter::speed - character.velocity)
				* acceleration * deltaTime;
		}
		else if (character.state == CompCharacter::State::IDLE) {
			character.velocity -= character.velocity * acceleration * deltaTime;
		}

		character.position += character.velocity * deltaTime;
	});
}

void characterSyncSystem(entt::registry& world, const Network& network, float deltaTime) {

}