#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#include "vec2.h"
#include "globals.h"

struct ParticleProperties {
	float mass;
	float viscosity;
	float restDensity;
	float id;
	glm::vec4 color;

	ParticleProperties() = default;
	ParticleProperties(float m, float v, int _id, glm::vec4 _color) :
		mass(m), viscosity(v), id(_id), restDensity(constants::REST_DENSITY * m), color(_color) { }
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
};