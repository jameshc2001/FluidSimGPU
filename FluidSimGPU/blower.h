#pragma once

#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>

#include "cereal/cereal.hpp"
#include "cereal/types/array.hpp"

#include "globals.h"
#include "shader.h"

class Blower {
private:
	glm::vec2 sourcePosition;
	glm::vec2 centre;
	float length;
	float angle;
	bool parallel;
	bool behindEnd;

	std::array<glm::vec4, 8> vertexData;
	std::array<unsigned int, 6> vertexIndices = { 0, 2, 1, 2, 3, 1 };
	unsigned int VAO, EBO, VBO;

	void findCentre(glm::vec2 start1, glm::vec2 end1, glm::vec2 start2, glm::vec2 end2);

	void updateGPU(Shader* predictShader, glm::vec2 blowerDir, glm::vec2 AABBmin, glm::vec2 AABBmax);

public:
	float sourceWidth;
	float endWidth;
	float strength;
	bool on = false;
	bool visible = false;

	Blower() = default;
	Blower(glm::vec2 _sourcePosition, float _sourceWidth, float _length, float _endWidth,
		float _strength, float _angle, Shader* _predictShader);

	void setSource(Shader* predictShader, glm::vec2 source);
	void setEnd(Shader* predictShader, glm::vec2 end);

	void deleteBuffers();
	void completeSetup(Shader* predictShader);
	void setupBlower(Shader* predictShader);
	void render(Shader* pointShader);

	template<class Archive> void serialize(Archive& archive) {
		archive(sourcePosition, centre, length, angle, parallel, behindEnd, vertexData,
			vertexIndices, VAO, EBO, VBO, sourceWidth, endWidth, strength, on, visible);
	}
};