#pragma once

#define _USE_MATH_DEFINES

#include "imgui/imgui.h"

#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cereal/cereal.hpp"

namespace glm {
	template<class Archive> void serialize(Archive& archive, glm::vec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::vec4& v) { archive(v.x, v.y, v.z, v.w); }
}

template<class Archive> void serialize(Archive& archive, ImVec4& v) { archive(v.x, v.y, v.z, v.w); }


namespace guiVariables {
	inline int editMode = 0;
	inline int selectedFluid = 0;

	inline float gravity = -9.81f;

	inline int spawnSpeed = 5;
	inline float spawnInterval = 0.05f; //in secdonds
	inline float spawnRadius = 40.0f;
	inline float deleteRadius = 40.0f;

	inline bool lineSnap = false;

	inline int damSize = 10;
	inline glm::vec2 damPosition = glm::vec2(640, 360);

	inline float diseaseDistribution = 0;

	inline ImVec4 backgroundColor = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
	inline ImVec4 geometryColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	inline bool pause = false;

	inline int selectedFileName = 1;
}

namespace constants {
	inline constexpr int SCREEN_WIDTH = 1280;
	inline constexpr int SCREEN_HEIGHT = 720;
	inline constexpr int TOOL_SCREEN_WIDTH = 300;
	inline constexpr int TOOL_SCREEN_HEIGHT = SCREEN_HEIGHT;

	inline constexpr int TIME_SAMPLES = 50;

	inline constexpr float LINE_WIDTH = 3.0f;

	inline constexpr int FONT_SIZE = 18;



	//SIMULATION CONSTANTS START HERE

	inline constexpr float UPDATE_INTERVAL = 1.0f / 60.0f;
	inline constexpr int SUBSTEPS = 10; //8 usually
	inline constexpr float DT = UPDATE_INTERVAL / SUBSTEPS;

	inline constexpr float GRAVITY = -9.81f; //standard setting for resets
	inline constexpr float GRAVITY_SCALE = 100.0f;
	inline constexpr float PARTICLE_RADIUS = 3.0f;
	inline constexpr float PARTICLE_DIAMETER = 2.0f * PARTICLE_RADIUS;
	inline constexpr float REST_DENSITY = 1.0f / (PARTICLE_DIAMETER * PARTICLE_DIAMETER);
	inline constexpr float RELAXATION = 0.001f;
	inline constexpr float BOUND_DAMPING = -0.5f;
	inline constexpr float MAX_VELOCITY = 2.0f * PARTICLE_RADIUS; //was 0.4f * particle radius

	inline constexpr float KERNEL_RADIUS = 3.25f * PARTICLE_RADIUS; //was 3.0 * prad
	inline constexpr float KR2 = KERNEL_RADIUS * KERNEL_RADIUS;
	inline constexpr float POLY6 = 4.0f / ((float)M_PI * KR2 * KR2 * KR2 * KR2); //kernel radius^8
	inline constexpr float SPIKY_GRAD = -30.0f / ((float)M_PI * KR2 * KR2 * KERNEL_RADIUS); //kernel radius^5

	inline constexpr float AP_CONSTANT = 0.1f; //usually 0.1f
	inline constexpr float AP_DISTANCE = 0.3f * KERNEL_RADIUS; //usually 0.3f * KR
	inline constexpr float AP_DISTANCE2 = AP_DISTANCE * AP_DISTANCE;
	inline constexpr int AP_POWER = 4;
	inline constexpr float AP_DENOMINATOR = POLY6 * (KR2 - AP_DISTANCE2) * (KR2 - AP_DISTANCE2) * (KR2 - AP_DISTANCE2);

	//dimensions of particle grids, +1 for when particle radius is not a multiple of width and height
	inline constexpr int P_X_CELLS = (int)(constants::SCREEN_WIDTH / constants::KERNEL_RADIUS) + 1;
	inline constexpr int P_Y_CELLS = (int)(constants::SCREEN_HEIGHT / constants::KERNEL_RADIUS) + 1;
	inline constexpr int P_NUM_CELLS = P_X_CELLS * P_Y_CELLS + 1; //the +1 is for figuring out if we have reached the end of last cell

	//color grid details, here the +1s are for corners on the top and right side of the screen
	inline constexpr float COLOR_RESOLUTION = PARTICLE_DIAMETER / 2.0f; //for best results choose multiple of width and height
	inline constexpr float C_RES_HALF = COLOR_RESOLUTION / 2.0f;
	inline constexpr float C_RADIUS = KERNEL_RADIUS / 1.0f; //radius for which we check particles when calculating color
	inline constexpr float C_RADIUS_2 = C_RADIUS * C_RADIUS;
	inline constexpr int C_X_CELLS = (int)(constants::SCREEN_WIDTH / COLOR_RESOLUTION) + 2; //+1 for corners, +1 for screen misalignment
	inline constexpr int C_Y_CELLS = (int)(constants::SCREEN_HEIGHT / COLOR_RESOLUTION) + 2;
	inline constexpr int C_NUM_CELLS = C_X_CELLS * C_Y_CELLS;

	//dimensions of line grid
	inline constexpr float LINE_GRID_RESOLUTION = 8.0f;
	inline constexpr int L_X_CELLS = (int)(constants::SCREEN_WIDTH / constants::LINE_GRID_RESOLUTION) + 1;
	inline constexpr int L_Y_CELLS = (int)(constants::SCREEN_HEIGHT / constants::LINE_GRID_RESOLUTION) + 1;
	inline constexpr int L_NUM_CELLS = L_X_CELLS * L_Y_CELLS + 1; //the +1 is for figuring out if we have reached the end of last cell

	inline constexpr int MAX_PARTICLE_TYPES = 6;
	inline constexpr int MAX_PARTICLES = 50000;
	inline constexpr int MAX_LINES = 512;

	//UTILITY CONSTANTS START HERER

	
	inline constexpr float SPAWN_RADIUS = 36.0f; //originally KR * 3
	inline constexpr int SPAWN_INTERVAL_FAST = 5; //in ticks
	inline constexpr float SPAWN_RADIUS_FAST = 48.0f; //originally KR * 4
	inline constexpr float DESPAWN_RADIUS = 48.0f; //originally KR * 4
}