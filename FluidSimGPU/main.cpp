#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h> //probably won't use this

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <iomanip>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "gui.h"
#include "globals.h"
#include "shader.h"
#include "particle_system.h"
#include "line.h"

using namespace constants;

GLFWwindow* window;

Shader lineShader;

ParticleSystem particleSystem;
GUI gui;

float frametime = 0;
float accumulator = 0;
float timeAtLastSpawn = 0;

bool dKeyDown = false;
bool fKeyDown = false;
bool rKeyDown = false;
bool iKeyDown = false;
bool oKeyDown = false;
bool uKeyDown = false;
bool pKeyDown = false;
bool wKeyDown = false;

bool leftMouseJustDown = false;
bool leftMouseDown = false;
glm::vec2 mousePos;

//line building variables
bool buildingLine = false;
glm::vec2 lineStart;
glm::vec2 lineEnd;

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void windowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
	gui.setScaling(xscale); //assume xscale == yscale
}

bool initialise() {
	const char* glsl_version = "#version 130"; //for imgui

	//initialise and configure glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //opengl 4.3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE); //high dpi scaling on windows and x11 systems
	//optionally enable MSAA, follow learnopengl for this

	//beg apple to let this code run
	//#ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//#endif

	//create window using glfw
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fluid Sim", NULL, NULL);
	if (window == NULL) {
		std::cout << "glfw window creation failed" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetWindowAspectRatio(window, SCREEN_WIDTH, SCREEN_HEIGHT);
	//glfwSwapInterval(1); //enable vsync, drastically drops cpu usage, could cause stutter with physics?
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetWindowContentScaleCallback(window, windowContentScaleCallback);
	//glfwSetMouseButtonCallback(window, mouseButtonCallback);

	//use glad to load all opengl function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "glad initialisation failed" << std::endl;
		glfwTerminate();
		return false;
	}

	//glEnable(GL_PROGRAM_POINT_SIZE);

	//setup dear imgui
	gui.initialise(window, &particleSystem);
	//do initial scaling
	float xscale, yscale;
	glfwGetWindowContentScale(window, &xscale, &yscale);
	gui.setScaling(xscale); //assume xscale == yscale

	//generate projection matrix
	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT, -1.0f, 1.0f);
	//load shaders
	lineShader.load("shaders/lineVertex.vert", "shaders/lineFragment.frag"); //NEED TO SET PROJECTION
	lineShader.use(); //very important
	glUniformMatrix4fv(glGetUniformLocation(lineShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set line width to reasonable size
	glLineWidth(4.0f);

	//initialisation successful
	return true;
}

//current time is used for timing particle spawning
void handleInput(float currentTime) {
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dKeyDown) {
		particleSystem.spawnDam(30, 0, SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2);
		dKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyDown) {
		particleSystem.spawnDam(30, 1, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
		fKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyDown) {
		particleSystem.resetParticles();
		rKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wKeyDown) {
		particleSystem.addLine(glm::vec2(200, 500), glm::vec2(600, 100));
		particleSystem.addLine(glm::vec2(600, 100), glm::vec2(1000, 300));
		wKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !iKeyDown) {
		particleSystem.drawParticles = !particleSystem.drawParticles;
		iKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oKeyDown) {
		particleSystem.drawMs = !particleSystem.drawMs;
		oKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && !uKeyDown) {
		particleSystem.calcColor = !particleSystem.calcColor;
		uKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pKeyDown) {
		particleSystem.performanceMode = !particleSystem.performanceMode;
		if (particleSystem.performanceMode) {
			particleSystem.wait = false;
			frametime = 1.0f / 30.0f;
		}
		else {
			frametime = 0.0f;
		}
		accumulator = 0.0f;
		pKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
		dKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
		wKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
		fKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
		rKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE) {
		iKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE) {
		oKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_RELEASE) {
		uKeyDown = false;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
		pKeyDown = false;
	}

	//handle mouse input
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMouseDown) {
		leftMouseDown = true;
		leftMouseJustDown = true;
	}
	else {
		leftMouseJustDown = false;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && leftMouseDown) {
		leftMouseDown = false;
	}

	if (leftMouseDown) { //i need to limit this to run independent off framerate
		//first calculate cursor position
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		//get screen size
		int winx, winy;
		glfwGetWindowSize(window, &winx, &winy);
		//convert to world coordinates
		x = (x / winx) * SCREEN_WIDTH;
		y = (1.0 - y / winy) * SCREEN_HEIGHT;
		mousePos = glm::vec2(x, y);

		if (guiVariables::editMode == 0) { //currently in fluid editing mode

			//figure out if enough time has passed to spawn another particle
			float timeSinceLastSpawn = currentTime - timeAtLastSpawn;

			if (timeSinceLastSpawn >= SPAWN_INTERVAL) {
				timeAtLastSpawn = currentTime;
				std::vector<Particle> particlesToAdd;
				for (int i = 0; i < guiVariables::spawnSpeed; i++) {
					glm::vec2 pos = glm::vec2(std::rand() / (float)RAND_MAX * guiVariables::spawnRadius, std::rand() / (float)RAND_MAX * guiVariables::spawnRadius);
					pos += mousePos - glm::vec2(guiVariables::spawnRadius / 2, guiVariables::spawnRadius / 2);
					particlesToAdd.push_back(Particle(pos.x, pos.y, guiVariables::selectedFluid));
				}
				particleSystem.addParticles(&particlesToAdd);
			}
		}
		//else { //currently in geometry editing mode
		//	if (!buildingLine) { //start building a line
		//		buildingLine = true;
		//		lineStart = mousePos;
		//		std::cout << "line has started being built" << std::endl;
		//	}
		//	else { //complete the line that is being built
		//		buildingLine = false;
		//		lineEnd = mousePos;
		//		particleSystem.addLine(lineStart, lineEnd);
		//		std::cout << "line has been built" << std::endl;
		//	}
		//}
	}

	if (leftMouseJustDown) {
		if (guiVariables::editMode == 1) {
			if (!buildingLine) { //start building a line
				buildingLine = true;
				lineStart = mousePos;
				std::cout << "line has started being built" << std::endl;
			}
			else { //complete the line that is being built
				buildingLine = false;
				lineEnd = mousePos;
				particleSystem.addLine(lineStart, lineEnd);
				std::cout << "line has been built" << std::endl;
			}
		}
	}
}

int main() {
	if (!initialise()) return -1;
	particleSystem.initialise();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClearColor(1.0f, 0.8f, 0.5f, 1.0f); //set background colour

	int currentSample = 0;
	float sampleSum = 0;

	float deltaTime = 0;
	float prevTime = 0;

	//main loop
	while (!glfwWindowShouldClose(window)) {
		
		//get time since last frame
		float currentTime = glfwGetTime();
		deltaTime = currentTime - prevTime;
		prevTime = currentTime;
		accumulator += deltaTime;

		//input handling here
		handleInput(currentTime);

		//sim updating here
		particleSystem.update(deltaTime);

		//std::cout << accumulator << "   " << frametime << std::endl;
		if (accumulator >= frametime) {
			//update gui
			gui.update();

			//rendering here
			glClear(GL_COLOR_BUFFER_BIT);
			particleSystem.render();
			gui.render(); //put ui ontop of particles

			//display new frame
			glfwSwapBuffers(window);
			accumulator -= frametime;
		}

		//poll new events
		glfwPollEvents();
	}

	glfwTerminate();
	particleSystem.close();
	return 0;
}