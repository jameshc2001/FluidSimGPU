#include "Sim.h"

using namespace constants;

Sim::Sim(Shader* _lineShader, Shader* _particlesShader) :
	lineShader(_lineShader), particlesShader(_particlesShader), particleSystem(_particlesShader, &lines) { }

void Sim::initialise() {
	//setup rendering stuff
	particleSystem.initialise();

	lines.reserve(100);

	//addLine(glm::vec2(300.0f, 100.0f), glm::vec2(700.0f, 300.0f));
}

void Sim::addLine(glm::vec2 v1, glm::vec2 v2) { //move this to line constructor?
	lines.push_back(Line(v1, v2, lineShader));
	Line* l = &lines.back();

	//now do bresenhams line draw algorithm to determine which cells could have collisions with the line
	//first convert to grid coordinates
	int x0 = floor(v1.x / LINE_GRID_RESOLUTION);
	int y0 = floor(v1.y / LINE_GRID_RESOLUTION);
	int x1 = floor(v2.x / LINE_GRID_RESOLUTION);
	int y1 = floor(v2.y / LINE_GRID_RESOLUTION);

	//use Alois Zingl's version of bresenham line drawing algorithm (from easy filter REFERENCE PROPERLY)
	int dx = abs(x1 - x0);
	int dy = -abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	int e2;

	auto lineGrid = particleSystem.lineGrid;

	while (true) {
		//first add this line to the current cell and surrounding cells
		for (int i = x0 - 1; i <= x0 + 1; i++) {
			for (int j = y0 - 1; j <= y0 + 1; j++) {
				if (i < 0 || j < 0 || i >= L_X_CELLS || j >= L_Y_CELLS) continue;
				if (lineGrid[i][j].size() > 0) {
					if (lineGrid[i][j].back() == l) continue; //already added line to this cell
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
}

void Sim::removeLine(int x, int y) {
	//Vec2 position(x, y);

	//int gridX = floor(x / PARTICLE_RADIUS);
	//int gridY = floor(y / PARTICLE_RADIUS);

	//for (auto& l : lineGrid[gridX][gridY]) {
	//	l->destroy = true;
	//}

	//lines.erase(std::remove_if(lines.begin(), lines.end(),
	//	[&](const Line& l) {
	//		return l.destroy;
	//	}), lines.end());

	//updateLineGrid();
}

void Sim::updateLines(int x, int y) {
	//int gridX = floor(x / PARTICLE_RADIUS);
	//int gridY = floor(y / PARTICLE_RADIUS);
	//for (auto& l : lineGrid[gridX][gridY]) {
	//	l->highlighted = true;
	//}
}

//this is ugly and slow, but the performance benefits for handling collisions is worth it
//also this is only called when a line is deleted, so not often at all
void Sim::updateLineGrid() {
	std::vector<Line> savedLines;
	for (Line l : lines) { //intentional copy
		savedLines.push_back(l);
	}
	lines.clear();

	for (int x = 0; x < L_X_CELLS; x++) {
		for (int y = 0; y < L_Y_CELLS; y++) {
			particleSystem.lineGrid[x][y].clear();
		}
	}

	for (auto& l : savedLines) {
		addLine(l.a, l.b);
	}
}

void Sim::spawnDam(int n, int properties, float x, float y) {
	//for (float i = x; i < x + n * KERNEL_RADIUS; i += KERNEL_RADIUS) {
	//	for (float j = y; j < y + n * KERNEL_RADIUS; j += KERNEL_RADIUS) {
	//		float randomness = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	//		particleSystem.particles.push_back(Particle(i + randomness, j, properties));
	//	}
	//}
	//std::cout << "\n" << particleSystem.particles.size() << "\n" << std::endl;

	particlesToAdd.clear();
	for (float i = x; i < x + n * KERNEL_RADIUS; i += KERNEL_RADIUS) {
		for (float j = y; j < y + n * KERNEL_RADIUS; j += KERNEL_RADIUS) {
			float randomness = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
			particlesToAdd.push_back(Particle(i + randomness, j, properties));
		}
	}
	particleSystem.addParticles(&particlesToAdd);
}

void Sim::spawnFromMouse(int x, int y, ParticleProperties* properties, bool fast) {
	//Uint32 elapsedTicks = SDL_GetTicks() - ticksAtLastSpawn;
	//if (fast) {
	//	if (elapsedTicks >= SPAWN_INTERVAL_FAST) {
	//		float xRandomness = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f;
	//		float yRandomness = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f;
	//		xRandomness *= SPAWN_RADIUS_FAST;
	//		yRandomness *= SPAWN_RADIUS_FAST;
	//		particles.push_back(Particle(x + xRandomness, y + yRandomness, properties));
	//		ticksAtLastSpawn = SDL_GetTicks();
	//	}
	//}
	//else {
	//	if (elapsedTicks >= SPAWN_INTERVAL) {
	//		float xRandomness = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f;
	//		float yRandomness = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f;
	//		xRandomness *= SPAWN_RADIUS;
	//		yRandomness *= SPAWN_RADIUS;
	//		particles.push_back(Particle(x + xRandomness, y + yRandomness, properties));
	//		ticksAtLastSpawn = SDL_GetTicks();
	//	}
	//}
}

void Sim::despawnFromMouse(int x, int y) {
	//Vec2 position(x, y);

	//particles.erase(std::remove_if(particles.begin(), particles.end(),
	//	[&](const Particle& p) {
	//		return (p.position - position).lengthSquared() < DESPAWN_RADIUS * DESPAWN_RADIUS;
	//	}), particles.end());

	//mouseX = x;
	//mouseY = y;
}

void Sim::saveState() {
	//savedParticles.clear();
	//savedLines.clear();
	//for (auto& p : particleSystem.particles) savedParticles.push_back(p);
	//for (auto& l : lines) savedLines.push_back(l);
	//for (int i = 0; i < MAX_PARTICLE_TYPES; i++) savedProperties[i] = particleSystem.particleProperties[i];
}

void Sim::loadState() {
	//particleSystem.particles.clear();
	//lines.clear();
	//updateLineGrid();
	//for (auto& p : savedParticles) particleSystem.particles.push_back(p);
	//for (auto& l : savedLines) addLine(l.a, l.b);
	//for (int i = 0; i < MAX_PARTICLE_TYPES; i++) particleSystem.particleProperties[i] = savedProperties[i];
}

void Sim::update(float deltaTime) {
	//below is perhaps the most important design decision made on the whole project
	//in a typical physics sim you wait until the accumulator as ticker over a certain predefined interval
	//once is has you perform the update. Typically you use a while (where I have used an if below), this
	//means the right number of updates is done if the accumulator >= 2 * update interval.
	//This can however lead to a snowball affect where each frame takes longer and demands even more physics
	//updates and leads to a crash. For the sake usability I have decided to just perform only one update whenever
	//the accumulator is over the interval. This slows down the simulation instead of causing lag, and more
	//importantly it means we don't waste CPU time waiting for accumulator to tick over into the next update.

	accumulator += deltaTime;
	if (accumulator >= UPDATE_INTERVAL) {
		particleSystem.update();
		accumulator -= UPDATE_INTERVAL;
	}
}

void Sim::render() {
	particleSystem.render();

	for (auto& l : lines) {
		l.render();
	}
}

void Sim::close() {
	particleSystem.close();

	for (auto& l : lines) {
		l.close();
	}
}