#include "particle_system.h"

using namespace constants;

ParticleSystem::ParticleSystem() {
	//create basic type
	particleProperties[0] = ParticleProperties(1, 0.01f, 0, glm::vec4(0.0f, 0.0f, 1.0f, 0.5f));
	particleProperties[1] = ParticleProperties(0.87f, 0.34f, 0, glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
	for (int i = 2; i < MAX_PARTICLE_TYPES; i++) {
		particleProperties[i] = ParticleProperties(1, 1.0f, 0, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}
}

void ParticleSystem::initialise() {
	glPointSize(PARTICLE_DIAMETER);

	//load point shader and set projection matrix
	particlePointShader.load("shaders/particlePoint.vert", "shaders/particlePoint.frag");
	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT, -1.0f, 1.0f);
	particlePointShader.use();
	glUniformMatrix4fv(glGetUniformLocation(particlePointShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//similarly for ms render shader
	msShader.load("shaders/ms.vert", "shaders/ms.frag");
	msShader.use();
	glUniformMatrix4fv(glGetUniformLocation(msShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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

	//line rendering shader
	lineShader.load("shaders/lineVertex.vert", "shaders/lineFragment.frag");
	lineShader.use();
	glUniformMatrix4fv(glGetUniformLocation(lineShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//remove particles shader
	removeShader.loadCompute("shaders/compute/utility/remove.comp");

	//counts diseased particles shader
	diseasedShader.loadCompute("shaders/compute/utility/diseased.comp");

	//measures physical distribution of diseased particles
	measureShader.loadCompute("shaders/compute/utility/measure.comp");


	predictShader.use();
	predictShader.setFloat("lineGridRes", LINE_GRID_RESOLUTION);
	predictShader.setInt("lineXCells", L_X_CELLS);
	applyShader.use();
	applyShader.setFloat("lineGridRes", LINE_GRID_RESOLUTION);
	applyShader.setInt("lineXCells", L_X_CELLS);


	setVectorField();


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

	glGenBuffers(1, &lineSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO); //maybe unnecessary
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lineSSBO);


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

	
	//setup buffers for line rendering, unlike particles this is all done cpu side
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &lineVBO);
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //vertex position as vec2
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 2)); //color as vec3
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	//setup buffers for delete line rendering, unlike particles this is all done cpu side
	glGenVertexArrays(1, &deleteLineVAO);
	glGenBuffers(1, &deleteLineVBO);
	glBindVertexArray(deleteLineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, deleteLineVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //vertex position as vec2
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 2)); //color as vec3
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleProperties) * particleProperties.size() + sizeof(diseased), NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticleProperties) * particleProperties.size(), &particleProperties[0]); //properties
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleProperties) * particleProperties.size(), sizeof(diseased), &diseased[0]); //diseased

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
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * C_NUM_CELLS + sizeof(glm::vec4) * C_NUM_CELLS, NULL, GL_DYNAMIC_DRAW);

	//set size of msSSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, msSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(VertexData)* C_NUM_CELLS * 9, NULL, GL_DYNAMIC_DRAW);

	//set size of lineSSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Line) * MAX_LINES + sizeof(int) * L_NUM_CELLS + sizeof(int) * 100000, NULL, GL_STATIC_DRAW); //dont know how big so make it super big

	//setup blower
	blower = Blower(glm::vec2(20, 500), 100, 400, 300, 5, -3.14f/2.0f, &predictShader);

	//setup fluid names
	fluidNames[0] = "Water";
	fluidNames[1] = "Oil";
	for (int i = 2; i < fluidNames.size(); i++) {
		fluidNames[i] = "Unnamed Fluid";
	}

	saveState(false);
}

void ParticleSystem::addParticles(std::vector<Particle>* particlesToAdd) {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, particles * sizeof(Particle), particlesToAdd->size() * sizeof(Particle), &(*particlesToAdd)[0]);
	particles += (int)particlesToAdd->size();
	if (diseased[(*particlesToAdd)[0].properties] == 1) diseasedParticles += (int)particlesToAdd->size();
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);
}

void ParticleSystem::removeParticles(glm::vec2 position, float radius) {
	removeShader.use();

	//grid buffer helps us delete particles and keep track of deletion of diseased particles
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	int data[] = { particles, 0 };
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * 2, &data[0]);

	removeShader.setFloat("deleteRadius", radius);
	removeShader.setVec2("deletePosition", position);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &particles);
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);

	int diseasedDeleted;
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int), sizeof(int), &diseasedDeleted);
	diseasedParticles -= diseasedDeleted;
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
	diseasedParticles = 0;
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);
	//also need to clear visual buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, msSSBO);
	GLint zero = 0;
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointSSBO);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
}

void ParticleSystem::resetGeometry() {
	if (numLines != 0) {
		//clear lineGrid, must be a faster way than this
		for (int i = 0; i < L_X_CELLS; i++) {
			for (int j = 0; j < L_Y_CELLS; j++) {
				lineGrid[i][j].clear();
			}
		}

		numLines = 0;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO);
		GLint zero = 0;
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
	}
}

template<class Archive> void SaveState::serialize(Archive& archive) {
	archive(savedNumOfLines, savedLines, savedLineGrid, savedProperties, savedDiseased,
		savedNumOfParticles, savedNumOfDiseased, savedParticles, savedBlower, savedFluidNames, savedSelectedFluid,
		savedSpawnSpeed, savedSpawnInterval, savedSpawnRadius, savedDeleteRadius, savedDamSize,
		savedDamPos, savedGravityEnabled, savedWindEnabled, savedGravityStrength, savedWindStrength,
		savedWindCentre, savedDrawMode, savedPerformanceMode, savedPause, savedBackgroundColor,
		savedGeometryColor);
}

void ParticleSystem::saveState(bool file) {
	SaveState& state = file ? fileState : quickState;

	state.savedNumOfLines = numLines;
	state.savedLines = lines;

	for (int x = 0; x < L_X_CELLS; x++) {
		for (int y = 0; y < L_Y_CELLS; y++) {
			state.savedLineGrid[x][y] = lineGrid[x][y];
		}
	}

	state.savedBlower = blower;
	state.savedFluidNames = fluidNames;

	state.savedProperties = particleProperties;
	state.savedDiseased = diseased;
	state.savedNumOfParticles = particles;
	state.savedNumOfDiseased = diseasedParticles;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, MAX_PARTICLES * sizeof(Particle), &state.savedParticles[0]);

	//now save all the property/settings info
	state.savedSelectedFluid = guiVariables::selectedFluid;
	state.savedSpawnSpeed = guiVariables::spawnSpeed;
	state.savedSpawnInterval = guiVariables::spawnInterval;
	state.savedSpawnRadius = guiVariables::spawnRadius;
	state.savedDeleteRadius = guiVariables::deleteRadius;
	state.savedDamSize = guiVariables::damSize;
	state.savedDamPos = guiVariables::damPosition;

	//need to call functions to update a lot of this stuff GPU side
	state.savedGravityEnabled = gravityEnabled;
	state.savedWindEnabled = windEnabled;
	state.savedGravityStrength = guiVariables::gravity;
	state.savedWindStrength = windStrength;
	state.savedWindCentre = windCentre;

	state.savedDrawMode = drawMode;
	state.savedPerformanceMode = performanceMode;
	state.savedPause = guiVariables::pause;

	state.savedBackgroundColor = guiVariables::backgroundColor;
	state.savedGeometryColor = guiVariables::geometryColor;
}

void ParticleSystem::loadState(bool file) {
	SaveState& state = file ? fileState : quickState;

	numLines = state.savedNumOfLines;
	lines = state.savedLines;
	for (int x = 0; x < L_X_CELLS; x++) {
		for (int y = 0; y < L_Y_CELLS; y++) {
			lineGrid[x][y] = state.savedLineGrid[x][y];
		}
	}

	blower.deleteBuffers();//must do this first for safety
	blower = state.savedBlower;
	blower.completeSetup(&predictShader);
	fluidNames = state.savedFluidNames;

	particleProperties = state.savedProperties;
	diseased = state.savedDiseased;
	particles = state.savedNumOfParticles;
	diseasedParticles = state.savedNumOfDiseased;
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &particles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, MAX_PARTICLES * sizeof(Particle), &state.savedParticles[0]);

	//update which particles are considered 'diseased'
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, propertiesSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleProperties) * particleProperties.size(), sizeof(diseased), &diseased[0]); //diseased

	updateProperties();

	if (numLines != 0) {
		updateLinesGPU();
	}
	else {
		//clear buffer
		glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
		GLint zero = 0;
		glClearBufferData(GL_ARRAY_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
		//also need to clear shader buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
	}

	//now load all the property/settings info
	guiVariables::selectedFluid = state.savedSelectedFluid;
	guiVariables::spawnSpeed = state.savedSpawnSpeed;
	guiVariables::spawnInterval = state.savedSpawnInterval;
	guiVariables::spawnRadius = state.savedSpawnRadius;
	guiVariables::deleteRadius = state.savedDeleteRadius;
	guiVariables::damSize = state.savedDamSize;
	guiVariables::damPosition = state.savedDamPos;

	//need to call functions to update a lot of this stuff GPU side
	gravityEnabled = state.savedGravityEnabled;
	windEnabled = state.savedWindEnabled;
	guiVariables::gravity = state.savedGravityStrength;
	windStrength = state.savedWindStrength;
	windCentre = state.savedWindCentre;

	drawMode = state.savedDrawMode;
	performanceMode = state.savedPerformanceMode;
	guiVariables::pause = state.savedPause;

	guiVariables::backgroundColor = state.savedBackgroundColor;
	guiVariables::geometryColor = state.savedGeometryColor;

	//update stuff gpu side
	updateGravity();
	updateLineColor();
	setVectorField();
}

void ParticleSystem::saveToFile(std::string path) {
	saveState(true);
	std::ofstream file(path, std::ios::binary);
	cereal::BinaryOutputArchive ar(file);
	ar(fileState);
}

void ParticleSystem::loadFromFile(std::string path) {
	std::ifstream file(path, std::ios::binary);
	cereal::BinaryInputArchive ar(file);
	ar(fileState);
	loadState(true);
}

//send particle properties to GPU, called by gui
void ParticleSystem::updateProperties() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, propertiesSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ParticleProperties) * particleProperties.size(), &particleProperties[0]);
}

void ParticleSystem::updateDiseased() {
	//update which particles are considered 'diseased'
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, propertiesSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(ParticleProperties) * particleProperties.size(), sizeof(diseased), &diseased[0]); //diseased

	//update number of diseased particles
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	int data = 0;
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &data);

	diseasedShader.use();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &diseasedParticles);
}

void ParticleSystem::updateGravity() {
	glBindBuffer(GL_UNIFORM_BUFFER, simUBO);
	float g = guiVariables::gravity * GRAVITY_SCALE;
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(int) * simInts.size() + sizeof(float) * 11, sizeof(float), &g);
}

void ParticleSystem::setVectorField() {
	predictShader.use();
	predictShader.setBool("gravityEnabled", gravityEnabled);
	predictShader.setBool("windEnabled", windEnabled);
	predictShader.setVec2("centre", windCentre);
	predictShader.setVec2("strength", windStrength);
}

void ParticleSystem::measureDiseaseDistribution() {
	//calculate distance from each disease particle to every other disease particle
	//divide this by the total number of distances calculated
	//O(n^2) sadly, atleast we can do it in parallel on the GPU

	//first element used for total distance, second used for counting number of distances
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	int data[] = { 0, 0 };
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * 2, &data[0]);

	//call shader
	measureShader.use();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

	//calculate result
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * 2, &data[0]); //data[0] right?
	if (data[1] == 0) guiVariables::diseaseDistribution = 0; //avoid divide by zero
	else guiVariables::diseaseDistribution = abs(((float)data[0] / 100.0f) / (float)data[1]);
}

void ParticleSystem::addLineFromMouse(glm::vec2 v1, glm::vec2 v2) {
	glm::vec2 dir = v2 - v1;
	dir = glm::normalize(dir);
	v1 -= constants::PARTICLE_RADIUS * dir;
	v2 += constants::PARTICLE_RADIUS * dir;
	addLine(v1, v2, true);
}

void ParticleSystem::addLine(glm::vec2 v1, glm::vec2 v2, bool updateGPU) {
	lines[numLines] = Line(v1, v2);
	int l = numLines;

	//now do bresenhams line draw algorithm to determine which cells could have collisions with the line
	//first convert to grid coordinates
	int x0 = (int)floor(v1.x / LINE_GRID_RESOLUTION);
	int y0 = (int)floor(v1.y / LINE_GRID_RESOLUTION);
	int x1 = (int)floor(v2.x / LINE_GRID_RESOLUTION);
	int y1 = (int)floor(v2.y / LINE_GRID_RESOLUTION);

	//use Alois Zingl's version of the bresenham line drawing algorithm from Easy Filter:
	//http://members.chello.at/easyfilter/bresenham.html
	int dx = abs(x1 - x0);
	int dy = -abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	int e2;

	while (true) {
		//first add this line to the current cell and surrounding cells
		for (int i = x0 - 1; i <= x0 + 1; i++) {
			for (int j = y0 - 1; j <= y0 + 1; j++) {
				if (i < 0 || j < 0 || i >= L_X_CELLS || j >= L_Y_CELLS) continue; //out of bounds check
				if (lineGrid[i][j].size() > 0) { //for some reason, this cause collision issues
					//if (lineGrid[i][j].back() == l) continue; //already added line to this cell
				}
				lineGrid[i][j].push_back(l);
			}
		}

		//progress the line drawing algorithm
		e2 = 2 * err;
		if (e2 >= dy) {
			if (x0 == x1) break;
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			if (y0 == y1) break;
			err += dx;
			y0 += sy;
		}
	}

	//we are done, increment line array pointer
	numLines++;

	//update this all on the gpu side
	if (updateGPU) updateLinesGPU();
}

void ParticleSystem::removeLine(glm::vec2 position) {
	//get grid coordinates of mouse position
	int x = (int)floor(position.x / LINE_GRID_RESOLUTION);
	int y = (int)floor(position.y / LINE_GRID_RESOLUTION);

	if (lineGrid[x][y].size() == 0) return; //no line at position

	int lineToRemove = lineGrid[x][y][0]; //remove first line at position
	
	//remove said line
	if (numLines == 1) {
		numLines = 0;
	}
	else if (lineToRemove == numLines - 1) {
		numLines--;
	}
	else {
		lines[lineToRemove] = lines[numLines - 1];
		numLines--;
	}

	//clear lineGrid, must be a faster way than this
	for (int i = 0; i < L_X_CELLS; i++) {
		for (int j = 0; j < L_Y_CELLS; j++) {
			lineGrid[i][j].clear();
		}
	}

	//special case
	if (numLines == 0) {
		//clear buffer
		glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
		GLint zero = 0;
		glClearBufferData(GL_ARRAY_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
		//also need to clear shader buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero);
		return;
	}

	int oldNumLines = numLines;
	numLines = 0;
	//re-add lines
	for (int i = 0; i < oldNumLines; i++) {
		addLine(lines[i].a, lines[i].b, false);
	}

	//update gpu
	updateLinesGPU();
}

glm::vec2 ParticleSystem::getNearestVertex(glm::vec2 position) {
	//get grid coordinates of mouse position
	int x = (int)floor(position.x / LINE_GRID_RESOLUTION);
	int y = (int)floor(position.y / LINE_GRID_RESOLUTION);

	if (lineGrid[x][y].size() == 0) return glm::vec2(-1, -1);

	Line l = lines[lineGrid[x][y][0]];
	if (glm::distance(l.a, position) < glm::distance(l.b, position)) return l.a;
	else return l.b;
}

void ParticleSystem::setDeleteLine(glm::vec2 position) {
	//get grid coordinates of mouse position
	int x = (int)floor(position.x / LINE_GRID_RESOLUTION);
	int y = (int)floor(position.y / LINE_GRID_RESOLUTION);

	//check if in bounds (mouse can do weird things) or if line selected
	if (x < 0 || y < 0 || x >= L_X_CELLS || y >= L_Y_CELLS || lineGrid[x][y].size() == 0) {
		//set prev line back
		if (prevDeleteLine != -1) {
			setLineColor(prevDeleteLine, glm::vec3(guiVariables::geometryColor.x, guiVariables::geometryColor.y, guiVariables::geometryColor.z));
			prevDeleteLine = -1;
		}
		return;
	}

	int lineToRender = lineGrid[x][y][0]; //render first line at position

	//check if its the same line as previous, setting previous back to black if necessary
	if (lineToRender != prevDeleteLine && prevDeleteLine != -1) {
		setLineColor(prevDeleteLine, glm::vec3(guiVariables::geometryColor.x, guiVariables::geometryColor.y, guiVariables::geometryColor.z));
		prevDeleteLine = -1;
	}

	if (lineToRender != prevDeleteLine) {
		setLineColor(lineToRender, glm::vec3(1, 0, 0));//red, make it inverse of geometry color?
		prevDeleteLine = lineToRender;
	}
}

void ParticleSystem::setLineColor(int line, glm::vec3 color) {
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 2 * line * 5 * sizeof(float) + 2 * sizeof(float), 3 * sizeof(float), &color[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 2 * line * 5 * sizeof(float) + (2 + 5) * sizeof(float), 3 * sizeof(float), &color[0]);
}

void ParticleSystem::renderLine(glm::vec2 a, glm::vec2 b) {
	std::vector<LineVertexData> linevd;
	glm::vec3 color = glm::vec3(guiVariables::geometryColor.x, guiVariables::geometryColor.y, guiVariables::geometryColor.z);
	LineVertexData vd = { a, color };
	linevd.push_back(vd);
	vd = { b, color };
	linevd.push_back(vd);
	glBindVertexArray(deleteLineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, deleteLineVBO);
	glBufferData(GL_ARRAY_BUFFER, 10 * sizeof(float), &linevd[0], GL_DYNAMIC_DRAW);
	lineShader.use();
	glDrawArrays(GL_LINES, 0, 2);
}

void ParticleSystem::updateLinesGPU() {
	//first generate vertex data
	std::vector<LineVertexData> linevd;
	glm::vec3 color = glm::vec3(guiVariables::geometryColor.x, guiVariables::geometryColor.y, guiVariables::geometryColor.z);
	for (int i = 0; i < numLines; i++) {
		Line* line = &lines[i];
		LineVertexData vd = { line->a, color };
		linevd.push_back(vd);
		vd = { line->b, color };
		linevd.push_back(vd);
	}
	glBindVertexArray(lineVAO); //probably don't need this line
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertexData) * numLines * 2, &linevd[0], GL_STATIC_DRAW); //*2 because each line has 2 vertices
	glBindVertexArray(0);

	//prepare line data for GPU
	std::vector<int> lineIndexes; //points to lines in line array
	std::vector<int> lineCellStart; //points to start of cells in lineIndexes
	for (int y = 0; y < L_Y_CELLS; y++) {
		for (int x = 0; x < L_X_CELLS; x++) {
			int cellID = y * L_X_CELLS + x; //note how 1D cell ID is calculated
			lineCellStart.push_back((int)lineIndexes.size()); //current cell starts here
			for (int line : lineGrid[x][y]) {
				lineIndexes.push_back(line);
			}
		}
	}

	//send lines, lineIndexes and lineCellStart to GPU
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lineSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Line) * MAX_LINES, &lines[0]); //send lines
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(Line) * MAX_LINES, sizeof(int) * lineCellStart.size(), &lineCellStart[0]); //send cell start
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(Line) * MAX_LINES + sizeof(int) * lineCellStart.size(), sizeof(int) * lineIndexes.size(), &lineIndexes[0]); //send indexes
}

void ParticleSystem::updateGridGPU() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridCellSSBO);
	GLint zero = 0;
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED, GL_INT, &zero); //set grid data to all 0s

	//calculate number of particles in each cell and store each particles individual offset for later
	prepareShader.use();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1); //generate counters and copy particles over

	//gridCellSSBO still binded. Here we copy counters over to 2nd section of gridData array
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * P_NUM_CELLS, sizeof(int) * P_NUM_CELLS);

	//do prefix sum on list of particle cell amounts see wikipedia for algorithm
	//note: I do a conversion to exclusive prefix on the final iteration
	prefixIterationShader.use();
	prefixIterationShader.setInt("finalPrefixIteration", 0);
	int flag = 1;
	int limit = (int)ceil(log2(P_NUM_CELLS));
	for (int i = 0; i < limit; i++) {
		prefixIterationShader.setInt("i", i);
		prefixIterationShader.setInt("flag", flag);
		if (i == limit - 1) prefixIterationShader.setInt("finalPrefixIteration", 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((int)ceil((float)P_NUM_CELLS / 1024.0f), 1, 1);
		if (flag == 0) flag = 1;
		else flag = 0;
	}

	//using the exclusive prefix list and previously computed indiviual offsets to move
	//particles into their correction positions within the particle buffer
	sortShader.use();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);
}

void ParticleSystem::generateVertexData() {
	if (!performanceMode || !wait) {
		if (drawMode == 0) {
			colorShader.use(); //generates colour values
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glDispatchCompute((int)ceil((float)C_NUM_CELLS / 1024.0f), 1, 1);
			genMSVerticesShader.use(); //convertes colour grid into vertices for rendering
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glDispatchCompute((int)ceil((float)C_NUM_CELLS / 1024.0f), 1, 1);
		}
		else {
			genVerticesShader.use(); //simply creates vertex for each particle
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);
		}
	}
	wait = !wait;
}

void ParticleSystem::updateParticles() {
	if (particles == 0) return;

	//sort particles for efficiency
	updateGridGPU();

	//render before updating, this may seem odd but rendering appears better when using
	//the most up to date grid information so it is best to fall a frame behind
	generateVertexData();

	//now update simulation, this is done in a loop because we can use the current grid data
	//for multiple updates because particle neighbours will only change a small amount
	for (int substep = 0; substep < SUBSTEPS; substep++) {
		//apply external forces and move particles to predicted (naive) position
		predictShader.use();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

		//calculate lambdas using sph density constraint function
		calcLamdasShader.use();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

		//use lambdas and apply artifical pressure to update initial prediction of position
		//also calculates velocity of particles using previous position and predicted position
		improveShader.use();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);

		//apply viscosity using velocity values and then set the particles actual position
		//to the predicted position
		applyShader.use();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((int)ceil((float)particles / 1024.0f), 1, 1);
	}

	particlesToRender = particles;
}

void ParticleSystem::update(float deltaTime) {
	accumulator += deltaTime;
	if (accumulator >= UPDATE_INTERVAL) {
		updateParticles();
		accumulator -= UPDATE_INTERVAL;
	}
	//if we fall more than one update behind ignore it, only consider fraction part
	//e.g. if we are 2.4 * UPDATE_INTERVAL behind, adjust so that we are 0.4 * UPDATE_INTERVAL behind
	//this prevents us from trying to catch up when it's not possible
	while (accumulator >= UPDATE_INTERVAL) accumulator -= UPDATE_INTERVAL;
}

void ParticleSystem::render() {
	if (particles != 0) {
		if (drawMode == 0) { //ms
			glBindVertexArray(msVAO);
			msShader.use();

			//each cell has 3 triangles, so 9 vertices
			glDrawArrays(GL_TRIANGLES, 0, C_NUM_CELLS * 9);
		}
		else { //particles
			glBindVertexArray(pointVAO);
			particlePointShader.use();
			glDrawArrays(GL_POINTS, 0, particlesToRender);
		}
	}

	//draw geometry on top of everything else
	glBindVertexArray(lineVAO);
	lineShader.use();
	glDrawArrays(GL_LINES, 0, numLines * 2); //2 vertices per line

	if (blower.visible) blower.render(&particlePointShader);

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