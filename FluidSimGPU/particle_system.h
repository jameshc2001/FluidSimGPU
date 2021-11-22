#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <algorithm>
#include <array>
#include <math.h>

#include "particle.h"
#include "globals.h"
#include "shader.h"
#include "line.h"

using namespace constants;

struct LineVertexData {
	glm::vec2 position;
	glm::vec3 color;
};

struct VertexData {
	glm::vec4 position;
	glm::vec4 color;
};

struct colorData {
	glm::vec2 position;
	float value;
	float density;
	glm::vec2 velocity;
	glm::vec2 rg;
	glm::vec2 ba;
};

class ParticleSystem {
private:
	Shader predictShader;
	Shader calcLamdasShader;
	Shader improveShader;
	Shader applyShader;
	Shader boundaryShader;
	Shader genVerticesShader;
	Shader colorShader;
	Shader genMSVerticesShader;

	unsigned int particleSSBO;
	unsigned int gridCellSSBO;
	unsigned int lineSSBO;
	unsigned int propertiesSSBO;
	unsigned int colorSSBO;
	unsigned int msSSBO;

	Shader lineShader;
	Shader particlePointShader;
	Shader msShader;

	unsigned int lineVAO;
	unsigned int lineVBO;
	unsigned int msVAO;
	unsigned int pointVAO;
	unsigned int pointSSBO;

	//setup initial simulation values to be sent to gpu
	std::array<int, 11> simInts = {0, P_NUM_CELLS, P_X_CELLS, P_Y_CELLS, SCREEN_WIDTH, SCREEN_HEIGHT, AP_POWER,
		C_NUM_CELLS, C_X_CELLS, C_Y_CELLS, 0};
	std::array<float, 16> simFloats = {MAX_VELOCITY, PARTICLE_RADIUS, BOUND_DAMPING, KERNEL_RADIUS, KR2, POLY6,
		SPIKY_GRAD, RELAXATION, AP_DENOMINATOR, AP_CONSTANT, DT, GRAVITY * GRAVITY_SCALE, COLOR_RESOLUTION,
		C_RES_HALF, C_RADIUS, C_RADIUS_2};

	unsigned int simUBO;

	Shader prepareShader;
	Shader prefixIterationShader;
	Shader sortShader;

	int particles = 0;
	int numLines = 0;
	float accumulator = 0;
	std::array<Line, constants::MAX_LINES> lines;
	std::vector<int> lineGrid[constants::L_X_CELLS][constants::L_Y_CELLS];

	//particle functions
	void updateGridGPU();
	void generateVertexData();
	void updateParticles();

	//line functions
	void updateLinesGPU();

public:
	bool drawParticles = false;
	bool drawMs = true;
	bool calcColor = true;
	bool performanceMode = false;
	int wait = false;

	std::array<ParticleProperties, constants::MAX_PARTICLE_TYPES> particleProperties;

	ParticleSystem();

	void addParticles(std::vector<Particle>* particlesToAdd);
	void spawnDam(int n, int properties, float x, float y);

	void resetParticles();
	void resetGeometry();
	void saveState();
	void loadState();

	void updateProperties();
	void updateGravity();

	void addLine(glm::vec2 v1, glm::vec2 v2);

	void initialise();
	void update(float deltaTime); //decide vertex stuff
	void render();
	void close();
};