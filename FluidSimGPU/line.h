#pragma once

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cereal/cereal.hpp"

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

		template<class Archive> void serialize(Archive& archive) {
			archive(a, b, ab, length, id);
		}
};
