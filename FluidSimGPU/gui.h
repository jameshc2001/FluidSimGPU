#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cstdio>
#include <iomanip>
#include <array>
#include <vector>
#include <filesystem>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "globals.h"
#include "particle_system.h"
#include "blower.h"

class GUI {
	private:
		GLFWwindow* window;
		ParticleSystem* particleSystem;;
		Blower* blower;
		float prevScaleFactor = 0;

		const std::string path = "scenarios/";
		std::vector<std::string> fileNames;
		std::string newFile;

		void updateFileNames();

		bool getAvgFrameTime = false;
		int samples = 0;
		float avgFrameTime = 0;


	public:
		GUI() = default;

		void setScaling(float scaleFactor);

		bool hasMouse();

		void initialise(GLFWwindow* _window, ParticleSystem* _particleSystem);
		void update();
		void render();
};