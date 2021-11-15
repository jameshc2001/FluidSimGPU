#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "particle.h"

class Line {
	public:
		glm::vec2 a;
		glm::vec2 b;
		glm::vec2 ab;
		float length;
		int id; //padding

		Line() = default;
		Line(glm::vec2 v1, glm::vec2 v2);

		//for this to work properly the line needs to be extended out both ways by r, maybe add this to the constructor?
		//this might not even look bad as it is...
		glm::vec2 particleCollision(Particle* p, float r, float damping);
};
