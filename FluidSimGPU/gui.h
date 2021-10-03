#pragma once

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

#include "globals.h"
#include "particle_system.h"

class GUI {
private:
	GLFWwindow* window;
	ParticleSystem* particleSystem;;
	float prevScaleFactor = 0;

public:
	GUI() = default;

	void setScaling(float scaleFactor);

	void initialise(GLFWwindow* _window, ParticleSystem* _particleSystem);
	void update();
	void render();
};