#include "particle_system.h"

using namespace constants;

ParticleSystem::ParticleSystem() {
	//create basic type
	particleProperties[0] = ParticleProperties(1, 0.1f, 0, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	particleProperties[1] = ParticleProperties(2, 5.0f, 0, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void ParticleSystem::initialise() {
	glPointSize(PARTICLE_DIAMETER);

	//load point shader and set projection matrix
	particlePointShader.load("shaders/particlePoint.vert", "shaders/particlePoint.frag");
	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT, -1.0f, 1.0f);
	particlePointShader.use();
	glUniformMatrix4fv(glGetUniformLocation(particlePointShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//setup compute shaders
	//simulation shaders
	predictShader.loadCompute("shaders/compute/update/predict.comp");
	calcLamdasShader.loadCompute("shaders/compute/update/calcLamdas.comp");
	improveShader.loadCompute("shaders/compute/update/improve.comp");
	applyShader.loadCompute("shaders/compute/update/apply.comp");

	//vertex generation shaders
	genVerticesShader.loadCompute("shaders/compute/render/genVertices.comp");
	colorShader.loadCompute("shaders/compute/render/colorField.comp");
	genMSVerticesShader.loadCompute("shaders/compute/render/msGenVertices.comp");

	//sorting shaders
	prepareShader.loadCompute("shaders/compute/sort/prepare.comp");
	prefixIterationShader.loadCompute("shaders/compute/sort/prefixIteration.comp");
	sortShader.loadCompute("shaders/compute/sort/sort.comp");


	glBindVertexArray(0); //probably useless

	glGenBuffers(1, &particleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO); //only need to do this once

	glGenBuffers(1, &gridCellSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gridCellSSBO);

	glGenBuffers(1, &propertiesSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, propertiesSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, propertiesSSBO);

	glGenBuffers(1, &colorSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colorSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, colorSSBO);


	//setup ssbo for vertex render
	glGenVertexArrays(1, &pointVAO);
	glGenBuffers(1, &pointSSBO);

	glBindVertexArray(pointVAO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, pointSSBO);

	glBindBuffer(GL_ARRAY_BUFFER, pointSSBO);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //vertex position
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 4)); //vertex color
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);


	//setup ssbo for ms render
	glGenVertexArrays(1, &msVAO);
	glGenBuffers(1, &msSSBO);

	glBindVertexArray(msVAO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, msSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, msSSBO);

	glBindBuffer(GL_ARRAY_BUFFER, msSSBO);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); //vertex position
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float) * 4)); //vertex color
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);


	//setup uniform buffer for holding simulation values
	glGenBuffers(1, &simUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, simUBO);
	//first allocate the memory
	glBufferData(GL_UNIFORM_BUFFER, sizeof(int) * simInts.size() + sizeof(float) * simFloats.size(), NULL, GL_STATIC_DRAW);
	//now send the two arrays
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int) * simInts.size(), &simInts[0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int) * simInts.size(), sizeof(float) * simFloats.size(), &simFloats[0]);
	//unbind
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	//send particle properties to gpu
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, propertiesSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleProperties) * particleProperties.size() + sizeof(float) * MAX_PARTICLES, NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticleProperties)* particleProperties.size(), &particleProperties[0]);

	//setup particle buffer to be max size, multiply by 2 for particle sorting operations
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle) * 2, NULL, GL_DYNAMIC_DRAW);
	//From now on only use BufferSubData on particle ssbo

	//allocate grid data
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, P_NUM_CELLS * sizeof(int) * 3 + MAX_PARTICLES * sizeof(int), NULL, GL_DYNAMIC_DRAW);

	//allocate vertex ssbo
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VertexData) * MAX_PARTICLES, NULL, GL_DYNAMIC_DRAW);

	//set size of color field ssbo
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colorSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)* C_NUM_CELLS * 8, NULL, GL_DYNAMIC_DRAW);

	//set size of msSSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, msSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VertexData)* C_NUM_CELLS * 12, NULL, GL_DYNAMIC_DRAW);
}

void ParticleSystem::addParticles(std::vector<Particle>* particlesToAdd) {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, particles * sizeof(Particle), particlesToAdd->size() * sizeof(Particle), &(*particlesToAdd)[0]);
	particles += particlesToAdd->size();
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);
	std::cout << "\n" << particles << "\n" << std::endl;
}

void ParticleSystem::spawnDam(int n, int properties, float x, float y) {
	std::vector<Particle> particlesToAdd;
	for (float i = x; i < x + n * KERNEL_RADIUS; i += KERNEL_RADIUS) {
		for (float j = y; j < y + n * KERNEL_RADIUS; j += KERNEL_RADIUS) {
			float randomness = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			particlesToAdd.push_back(Particle(i + randomness, j, properties));
		}
	}
	addParticles(&particlesToAdd);
}

void ParticleSystem::resetParticles() {
	particles = 0;
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);
	std::cout << "\n" << particles << "\n" << std::endl;
}

//void printGridBuffer(unsigned int ssbo) {
//	std::array<int, P_NUM_CELLS * 4> data;
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
//	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * 4 * P_NUM_CELLS, &data[0]);
//
//	std::cout << std::endl;
//	std::cout << "GPU GRID BUFFER DATA START" << std::endl;
//
//	for (int i = 0; i < P_NUM_CELLS * 4; i++) {
//		switch (i) {
//		case 0: std::cout << "SECTION 1" << std::endl; break;
//		case P_NUM_CELLS: std::cout << std::endl << "SECTION 2" << std::endl; break;
//		case P_NUM_CELLS * 2: std::cout << std::endl << "SECTION 3" << std::endl; break;
//		case P_NUM_CELLS * 3: std::cout << std::endl << "SECTION 4" << std::endl; break;
//		}
//		std::cout << data[i] << ", ";
//	}
//	std::cout << std::endl;
//
//	std::cout << "GPU GRID BUFFER DATA END" << std::endl;
//	std::cout << std::endl;
//}

void ParticleSystem::updateGridGPU() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	GLint zero = 0;
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero); //set grid data to all 0s

	//calculate number of particles in each cell and store each particles individual offset for later
	prepareShader.use();
	glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1); //generate counters and copy particles over

	//gridCellSSBO still binded. Here we copy counters over to 2nd section of gridData array
	glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * P_NUM_CELLS, sizeof(int) * P_NUM_CELLS);

	//do prefix sum on list of particle cell amounts see wikipedia for algorithm
	//note: I do a conversion to exclusive prefix on the final iteration
	prefixIterationShader.use();
	prefixIterationShader.setInt("finalPrefixIteration", 0);
	int flag = 1;
	int limit = ceil(log2(P_NUM_CELLS));
	for (int i = 0; i < limit; i++) {
		prefixIterationShader.setInt("i", i);
		prefixIterationShader.setInt("flag", flag);
		if (i == limit - 1) prefixIterationShader.setInt("finalPrefixIteration", 1);
		glDispatchCompute(ceil((float)P_NUM_CELLS / 1024.0f), 1, 1);
		if (flag == 0) flag = 1;
		else flag = 0;
	}

	//using the exclusive prefix list and previously computed indiviual offsets to move
	//particles into their correction positions within the particle buffer
	sortShader.use();
	glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);
}

void ParticleSystem::printColorField(unsigned int ssbo) {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * C_NUM_CELLS, &data[0]);

	std::cout << std::endl;
	std::cout << "GPU COLOR FIELD BUFFER DATA START" << std::endl;

	for (int i = C_NUM_CELLS - 1; i >= 0; i--) {
		if (i % C_X_CELLS == 0) std::cout << std::endl << std::endl;
		std::cout << data[i] << " | ";
	}
	std::cout << std::endl;

	std::cout << "GPU COLOR FIELD BUFFER DATA END" << std::endl;
	std::cout << std::endl;
}

void ParticleSystem::printColorData() {
	printColorField(colorSSBO);
}

void ParticleSystem::updateParticles() {
	if (particles == 0) return;

	//sort particles for efficiency
	updateGridGPU();

	//render before updating, this may seem odd but rendering appears better when using
	//the most up to date grid information so it is best to fall a frame behind

	if (drawMs) {
		colorShader.use(); //generates colour values
		glDispatchCompute(ceil((float)C_NUM_CELLS / 1024.0f), 1, 1);

		genMSVerticesShader.use(); //convertes colour grid into vertices for rendering
		glDispatchCompute(ceil((float)C_NUM_CELLS / 1024.0f), 1, 1);
	}

	if (drawParticles) {
		genVerticesShader.use(); //simply creates vertex for each particle
		glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);
	}


	//reset final iteration, this is used for knowing when to save densities for color generating
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	int finalIteration = 0;
	glBufferSubData(GL_UNIFORM_BUFFER, 10 * sizeof(int), sizeof(int), &finalIteration);

	//now update simulation, this is done in another loop because we can use the current grid data
	//for multiple updates because particle neighbours will only change a small amount
	for (int substep = 0; substep < SUBSTEPS; substep++) {
		if (substep == SUBSTEPS - 1) {
			finalIteration = 1;
			glBufferSubData(GL_UNIFORM_BUFFER, 10 * sizeof(int), sizeof(int), &finalIteration);
		}

		//apply external forces and move particles to predicted (naive) position
		predictShader.use();
		glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);

		//calculate lambdas using sph density constraint function
		calcLamdasShader.use();
		glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);

		//use lambdas and apply artifical pressure to update initial prediction of position
		//also calculates velocity of particles using previous position and predicted position
		improveShader.use();
		glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);

		//apply viscosity using velocity values and then set the particles actual position
		//to the predicted position
		applyShader.use();
		glDispatchCompute(ceil((float)particles / 1024.0f), 1, 1);
		

		//interesting to explore how difficult/easy to per particle things (heat, reactions)
	}
}

void ParticleSystem::update(float deltaTime) {
	accumulator += deltaTime;
	if (accumulator >= UPDATE_INTERVAL) {
		updateParticles();
		accumulator -= UPDATE_INTERVAL;
	}
}

void ParticleSystem::render() {
	if (particles == 0) return;

	if (drawMs) {
		glBindVertexArray(msVAO);
		glBindBuffer(GL_ARRAY_BUFFER, msSSBO);
		particlePointShader.use(); //need to make specific shader for ms
		glDrawArrays(GL_TRIANGLES, 0, C_NUM_CELLS * 12);
	}

	if (drawParticles) {
		glBindVertexArray(pointVAO);
		glBindBuffer(GL_ARRAY_BUFFER, pointSSBO);
		particlePointShader.use();
		glDrawArrays(GL_POINTS, 0, particles);
	}

	glBindVertexArray(0);
}

void ParticleSystem::close() {
	glDeleteVertexArrays(1, &pointVAO);
	glDeleteBuffers(1, &gridCellSSBO);
	glDeleteBuffers(1, &particleSSBO);
	glDeleteBuffers(1, &propertiesSSBO);
	glDeleteBuffers(1, &pointSSBO);
	glDeleteBuffers(1, &simUBO);
}