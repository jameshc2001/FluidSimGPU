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
Shader particlesShader;

ParticleSystem particleSystem;
GUI gui;

bool dKeyDown = false;
bool fKeyDown = false;
bool rKeyDown = false;
bool iKeyDown = false;
bool oKeyDown = false;

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
	particlesShader.load("shaders/particlesVertex.vert", "shaders/particlesFragment.frag");
	particlesShader.use();
	glUniformMatrix4fv(glGetUniformLocation(particlesShader.id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//initialisation successful
	return true;
}

void handleInput() {
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
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		particleSystem.printColorData();
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !iKeyDown) {
		particleSystem.drawParticles = !particleSystem.drawParticles;
		iKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oKeyDown) {
		particleSystem.drawMs = !particleSystem.drawMs;
		oKeyDown = true;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
		dKeyDown = false;
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
		//update gui
		gui.update();
		
		float currentTime = glfwGetTime();
		deltaTime = currentTime - prevTime;
		prevTime = currentTime;

		float start = glfwGetTime();
		//input handling here
		handleInput();

		//sim updating here
		particleSystem.update(deltaTime);

		//rendering here
		glClear(GL_COLOR_BUFFER_BIT);
		gui.render();
		particleSystem.render();

		//display new frame and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();

		float end = glfwGetTime();
		sampleSum += end - start;
		currentSample++;
		if (currentSample == TIME_SAMPLES) {
			float frameTime = sampleSum / (float)constants::TIME_SAMPLES;
			frameTime *= 1000;
			//std::cout << frameTime << std::endl;
			//std::cout << "\r" << std::setw(6) << std::setfill('0') << frameTime << std::flush;
			sampleSum = 0;
			currentSample = 0;
		}
	}

	glfwTerminate();
	particleSystem.close();
	return 0;
}