#pragma once

#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

class Blower {
private:
	glm::vec2 sourcePosition;
	float length;
	float angle;

	glm::vec4 vertexData[8]; //even indices for coordinates and odd for colors
	unsigned int vertexIndices[6] = {0, 2, 1, 2, 3, 1};
	unsigned int VAO, EBO, VBO;

	Shader* pointShader;
	Shader* predictShader;

	void updateGPU(glm::vec2 blowerDir, glm::vec2 AABBmin, glm::vec2 AABBmax);

public:
	float sourceWidth;
	float endWidth;
	float strength;
	bool on = false;

	Blower() = default;
	Blower(glm::vec2 _sourcePosition, float _sourceWidth, float _length, float _endWidth,
		float _strength, float _angle, Shader* _pointShader, Shader* _predictShader);

	void setSource(glm::vec2 source);
	void setEnd(glm::vec2 end);

	void setupBlower();
	void render();
};