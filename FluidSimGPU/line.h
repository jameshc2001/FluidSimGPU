#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vec2.h"
#include "particle.h"
#include "shader.h"

class Line {
	private:
		bool angleOf45;
		float length;
		glm::vec2 ab;

		//rendering stuff
		Shader* shader;
		unsigned int VBO, VAO;

	public:
		glm::vec2 a;
		glm::vec2 b;

		Line() = default;
		Line(glm::vec2 v1, glm::vec2 v2, Shader* _shader);

		//for this to work properly the line needs to be extended out both ways by r, maybe add this to the constructor?
		//this might not even look bad as it is...
		glm::vec2 particleCollision(Particle* p, float r, float damping);

		void render();
		void close();
};
