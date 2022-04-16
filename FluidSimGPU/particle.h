#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#include "cereal/cereal.hpp"

#include "globals.h"

struct ParticleProperties {
	float mass;
	float viscosity;
	float restDensity;
	float stickiness;
	glm::vec4 color;

	ParticleProperties() = default;
	ParticleProperties(float m, float v, float _stickiness, glm::vec4 _color) :
		mass(m), viscosity(v), stickiness(_stickiness), restDensity(constants::REST_DENSITY * m), color(_color) { }

	template<class Archive>
	void serialize(Archive& archive) {
		archive(mass, viscosity, restDensity, stickiness, color);
	}
};

struct Particle {
	glm::vec2 position;
	glm::vec2 predictedPosition;
	glm::vec2 velocity;

	float lambda; //for positional density constraint
	int properties;

	Particle() = default;
	Particle(float x, float y, int _properties) :
		position(x, y), predictedPosition(0.0f, 0.0f), velocity(0.0f, 0.0f),
		lambda(0), properties(_properties) {}
	
	template<class Archive>
	void serialize(Archive& archive) {
		archive(position, predictedPosition, velocity, lambda, properties);
	}
};