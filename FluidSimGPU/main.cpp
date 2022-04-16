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

float timeAtLastSpawn = 0;

//bool pause = false;

bool hKeyDown = false;
bool showGui = true;

bool leftMouseJustDown = false;
bool leftMouseDown = false;
bool rightMouseJustDown = false;
bool rightMouseDown = false;
glm::vec2 mousePos;

//line building variables
bool buildingLine = false;
glm::vec2 lineStart;
glm::vec2 lineEnd;

//circle rendering variables
float circleVertices[] = {
	0.5f,  0.5f,
	0.5f, -0.5f,
	-0.5f, -0.5f,
	-0.5f,  0.5f
};
unsigned int circleIndices[] = {
	0, 1, 3, 1, 2, 3
};
unsigned int circleVBO, circleVAO, circleEBO;
Shader circleShader;
float xscale, yscale;


void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	xscale = (float)width / (float)SCREEN_WIDTH;
}

void windowContentScaleCallback(GLFWwindow* window, float _xscale, float _yscale) {
	gui.setScaling(_xscale); //assume xscale == yscale
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

	//create window using glfw
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fluid Sim", NULL, NULL);
	if (window == NULL) {
		std::cout << "glfw window creation failed" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetWindowAspectRatio(window, SCREEN_WIDTH, SCREEN_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetWindowContentScaleCallback(window, windowContentScaleCallback);

	//use glad to load all opengl function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "glad initialisation failed" << std::endl;
		glfwTerminate();
		return false;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	//setup dear imgui
	gui.initialise(window, &particleSystem);
	//do initial scaling
	glfwGetWindowContentScale(window, &xscale, &yscale);
	gui.setScaling(xscale); //assume xscale == yscale

	//generate projection matrix
	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT, -1.0f, 1.0f);
	//load shaders
	lineShader.load("shaders/lineVertex.vert", "shaders/lineFragment.frag");
	lineShader.use(); //very important
	lineShader.setMat4("projection", projection);

	//set line width to reasonable size
	glLineWidth(4.0f);

	//setup circle rendering
	glGenVertexArrays(1, &circleVAO);
	glGenBuffers(1, &circleVBO);
	glGenBuffers(1, &circleEBO);
	glBindVertexArray(circleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices, GL_DYNAMIC_DRAW); //why dynamic?
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(circleIndices), circleIndices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	circleShader.load("shaders/circleVertex.vert", "shaders/circleFragment.frag");
	circleShader.use();
	circleShader.setMat4("projection", projection);
	glm::mat4 identity(1.0f);
	circleShader.setMat4("model", identity);

	//initialisation successful
	return true;
}

void renderCircle(glm::vec2 position, float radius) {
	circleShader.use();
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0));
	model = glm::scale(model, glm::vec3(2*radius, 2*radius, 0));
	circleShader.setMat4("model", model);
	circleShader.setFloat("circleRadius", radius * xscale);
	circleShader.setVec2("circleCentre", position * xscale);
	glBindVertexArray(circleVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

//current time is used for timing particle spawning
void handleInput(float currentTime) {
	//check if Dear ImGUI is using the mouse, also works for keyboard
	if (gui.hasMouse()) return;

	//keyboard input
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hKeyDown) {
		showGui = !showGui;
		hKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
		hKeyDown = false;
	}

	//handle mouse input
	//left
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
	//right
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !rightMouseDown) {
		rightMouseDown = true;
		rightMouseJustDown = true;
	}
	else {
		rightMouseJustDown = false;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE && rightMouseDown) {
		rightMouseDown = false;
	}

	//first calculate cursor position
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	//get screen size
	int winx, winy;
	glfwGetWindowSize(window, &winx, &winy);
	//convert to world coordinates
	x = (x / winx) * SCREEN_WIDTH; //can use xscale for these i think
	y = (1.0 - y / winy) * SCREEN_HEIGHT;
	mousePos = glm::vec2(x, y);

	//check to color in line red for deletion
	if (guiVariables::editMode == 1) {
		particleSystem.setDeleteLine(mousePos);
	}

	if (leftMouseDown) {
		if (guiVariables::editMode == 0 && !guiVariables::pause) { //currently in fluid editing mode

			if (leftMouseJustDown) { //if mouse just pressed then we want to spawn in particles NOW!!!!
				timeAtLastSpawn = currentTime - guiVariables::spawnInterval;
			}

			//figure out if enough time has passed to spawn another particle
			float timeSinceLastSpawn = currentTime - timeAtLastSpawn; //makes this framerate independent

			while (timeSinceLastSpawn >= guiVariables::spawnInterval) {
				timeAtLastSpawn = currentTime;
				std::vector<Particle> particlesToAdd;
				for (int i = 0; i < guiVariables::spawnSpeed; i++) {
					glm::vec2 pos = glm::vec2(std::rand() / (float)RAND_MAX * guiVariables::spawnRadius, std::rand() / (float)RAND_MAX * guiVariables::spawnRadius);
					pos += mousePos - glm::vec2(guiVariables::spawnRadius / 2, guiVariables::spawnRadius / 2);
					particlesToAdd.push_back(Particle(pos.x, pos.y, guiVariables::selectedFluid));
				}
				particleSystem.addParticles(&particlesToAdd);
				timeSinceLastSpawn -= guiVariables::spawnInterval;
			}
		}
		else if (guiVariables::editMode == 2) { //blower mode
			particleSystem.blower.setSource(particleSystem.getPredictShader(), mousePos);
		}
	}

	if (leftMouseJustDown) { //ie, not held down
		if (guiVariables::editMode == 1) { //geometry mode
			if (!buildingLine) { //start building a line
				buildingLine = true;
				lineStart = mousePos;

				if (guiVariables::lineSnap) {
					glm::vec2 nearest = particleSystem.getNearestVertex(mousePos);
					if (nearest.x != -1) lineStart = nearest;
				}
			}
			else { //complete the line that is being built
				buildingLine = false;
				lineEnd = mousePos;

				if (guiVariables::lineSnap) {
					glm::vec2 nearest = particleSystem.getNearestVertex(mousePos);
					if (nearest.x != -1) lineEnd = nearest;
				}

				particleSystem.addLine(lineStart, lineEnd, true);
			}
		}
	}

	if (rightMouseDown) {
		if (guiVariables::editMode == 0 && !guiVariables::pause) { //fluid mode
			particleSystem.removeParticles(mousePos, guiVariables::deleteRadius);
			renderCircle(mousePos, guiVariables::deleteRadius);
		}
		else if (guiVariables::editMode == 2) { //blower mode
			particleSystem.blower.setEnd(particleSystem.getPredictShader(), mousePos);
		}
	}

	if (rightMouseJustDown) {
		if (guiVariables::editMode == 1) {
			if (!buildingLine) {
				particleSystem.removeLine(mousePos);
			}
			else { //cancel current line
				buildingLine = false;
			}
		}
	}
}

int main() {
	if (!initialise()) return -1;
	particleSystem.initialise();

	glClearColor(1.0f, 0.8f, 0.5f, 1.0f); //set background colour

	float deltaTime = 0;
	float prevTime = 0;

	//main loop
	while (!glfwWindowShouldClose(window)) {
		
		//get time since last frame
		float currentTime = (float)glfwGetTime();
		deltaTime = currentTime - prevTime;
		prevTime = currentTime;

		glClear(GL_COLOR_BUFFER_BIT); //bad for performance?

		//input handling here
		handleInput(currentTime);

		//sim updating here
		if (!guiVariables::pause) particleSystem.update(deltaTime);

		//update gui
		if (showGui) gui.update();

		//rendering here
		particleSystem.render();
		//draw line being build
		if (buildingLine) particleSystem.renderLine(lineStart, mousePos);
		if (showGui) gui.render(); //put ui ontop of particles

		//display new frame
		glfwSwapBuffers(window);

		//poll new events
		glfwPollEvents(); //really slow function
	}

	glfwTerminate();
	particleSystem.close();
	return 0;
}