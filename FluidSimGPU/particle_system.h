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
#include "blower.h"

using namespace constants;

struct LineVertexData {
	glm::vec2 position;
	glm::vec3 color;
};

struct VertexData {
	glm::vec4 position;
	glm::vec4 color;
};

struct colorData { //can i remove this?
	glm::vec2 position;
	float value;
	float density;
	glm::vec2 velocity;
	glm::vec2 rg;
	glm::vec2 ba;
};

struct SaveState {
	int savedNumOfLines;
	std::array<Line, MAX_LINES> savedLines;
	std::vector<int> savedLineGrid[constants::L_X_CELLS][constants::L_Y_CELLS];
	std::array<ParticleProperties, MAX_PARTICLE_TYPES> savedProperties;
	std::array<int, MAX_PARTICLE_TYPES> savedDiseased;
	int savedNumOfParticles;
	int savedNumOfDiseased;
	std::array<Particle, MAX_PARTICLES> savedParticles;
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
	Shader removeShader;
	Shader diseasedShader;
	Shader measureShader;

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
	unsigned int deleteLineVAO;
	unsigned int deleteLineVBO;
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
	int particlesToRender = 0;
	int diseasedParticles = 0;
	int numLines = 0;
	float accumulator = 0;
	std::array<Line, constants::MAX_LINES> lines;
	std::vector<int> lineGrid[constants::L_X_CELLS][constants::L_Y_CELLS];

	int prevDeleteLine = -1; //-1 indicates no prev line

	//particle functions
	void updateGridGPU();
	void generateVertexData();
	void updateParticles();

	//line functions
	void setLineColor(int line, glm::vec3 color);
	void updateLinesGPU();

	SaveState state;

public:
	bool drawParticles = false;
	bool drawMs = true;
	bool calcColor = true;
	bool performanceMode = false;
	int wait = false;
	int drawMode = 0;

	std::array<int, constants::MAX_PARTICLE_TYPES> diseased; //one to one of particleProperties

	//vector field settings
	bool gravityEnabled = true;
	bool windEnabled = false;
	glm::vec2 windCentre = glm::vec2(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	glm::vec2 windStrength = glm::vec2(0.01, 0.01);

	std::array<ParticleProperties, constants::MAX_PARTICLE_TYPES> particleProperties;
	Blower blower;

	ParticleSystem();

	void addParticles(std::vector<Particle>* particlesToAdd);
	void removeParticles(glm::vec2 position, float radius);
	void spawnDam(int n, int properties, float x, float y);

	void resetParticles();
	void resetGeometry();
	void saveState();
	void loadState();

	void updateProperties();
	void updateDiseased();
	void updateGravity();
	void updateLineColor() { updateLinesGPU(); } //faster way of doing this?
	void setVectorField();
	void measureDiseaseDistribution();

	void addLineFromMouse(glm::vec2 v1, glm::vec2 v2); //need to add bits to each end
	void addLine(glm::vec2 v1, glm::vec2 v2, bool updateGPU);
	void removeLine(glm::vec2 position);
	glm::vec2 getNearestVertex(glm::vec2 position); //return position of nearest line vertex (if close enough)
	int getParticles() { return particles; }
	int getDiseased() { return diseasedParticles; }

	void setDeleteLine(glm::vec2 position); //change its color to red
	void renderLine(glm::vec2 a, glm::vec2 b);

	void initialise();
	void update(float deltaTime); //decide vertex stuff
	void render();
	void close();
};