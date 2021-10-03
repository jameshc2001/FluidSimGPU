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

#include "particle_system.h"
#include "particle.h"
#include "line.h"
#include "globals.h"
#include "thread_pool.hpp"
#include "shader.h"

class Sim {
private:
	Shader* lineShader;
	Shader* particlesShader; //note the plural, we render all particles in one batch

	float accumulator = 0.0f;
	float prevTime = 0.0f;

	std::vector<Line> lines;
	std::vector<Particle> particlesToAdd;

	//saving/loading data
	std::vector<Particle> savedParticles;
	std::array<ParticleProperties, constants::MAX_PARTICLE_TYPES> savedProperties;
	std::vector<Line> savedLines;

	//simulation utility functions
	void addLine(glm::vec2 v1, glm::vec2 v2); //remove line cannot change ordering
	void removeLine(int x, int y);
	void updateLines(int x, int y); //check if line is highlighted
	void updateLineGrid();

	void spawnDam(int n, int properties, float x, float y);
	void spawnFromMouse(int x, int y, ParticleProperties* properties, bool fast);
	void despawnFromMouse(int x, int y);

	void removeAllParticles() {  }
	void removeAllGeometry() { lines.clear(); updateLineGrid(); }

	void saveState();
	void loadState();

	friend class GUI;

public:
	Sim(Shader* _lineShader, Shader* _particlesShader);

	ParticleSystem particleSystem;

	void initialise();
	void update(float deltaTime);
	void render();
	void close();

	void spawnDamExposed(int n, int propertiesIndex, float x, float y) {
		spawnDam(n, propertiesIndex, x, y);
	}
};