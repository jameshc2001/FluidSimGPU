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

using namespace constants;

struct VertexData {
	glm::vec4 position;
	glm::vec4 color;
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
	unsigned int propertiesSSBO;
	unsigned int colorSSBO;
	unsigned int msSSBO;

	Shader particlePointShader;
	Shader msShader;

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

	std::array<float, constants::MAX_PARTICLES * 6 * 5> vertexData;
	unsigned int VBO, VAO;

	int prevNumParticles = 0;
	bool particlesChanged = false; //if true we need to completely reallocate vertex buffer
	bool updateVertexData = false; //if true we need to update the vertex buffers data

	int particles = 0;
	std::array<ParticleProperties, constants::MAX_PARTICLE_TYPES> particleProperties;

	float accumulator = 0;

	void printColorField(unsigned int ssbo);
	std::array<float, C_NUM_CELLS> data; // for debugging

	//particle functions
	void updateGridGPU();
	void updateParticles();

public:
	bool drawParticles = false;
	bool drawMs = true;

	ParticleSystem();

	void addParticles(std::vector<Particle>* particlesToAdd);
	void spawnDam(int n, int properties, float x, float y);
	void resetParticles();
	void printColorData();

	void initialise();
	void update(float deltaTime); //decide vertex stuff
	void render();
	void close();
};