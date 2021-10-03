//This code is heavily based upon the shader class in LearnOpenGL's getting started
//tutorial. The original code is covered by the CC BY-NC 4.0 license, meaning I am
//free to use it here for a non-comerical purpose.

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

//a convienence class for loading shaders
class Shader {
private:
	bool loaded = false;
public:
	unsigned int id; //program id assigned by opengl

	Shader() = default;
	Shader(const char* vertexPath, const char* fragmentPath) {
		load(vertexPath, fragmentPath);
	}

	void loadCompute(const char* computePath) { //specifically for loading compute shaders
		std::string computeCode;
		std::ifstream cShaderFile;

		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			cShaderFile.open(computePath);
			std::stringstream cShaderStream;
			cShaderStream << cShaderFile.rdbuf();
			cShaderFile.close();
			computeCode = cShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			std::cout << "shader file(s) not successfully read" << std::endl;
		}

		const char* cShaderCode = computeCode.c_str();

		unsigned int compute;
		int success;
		char infoLog[512];

		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);

		//check we didnt mess it up
		glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(compute, 512, NULL, infoLog);
			std::cout << "compute shader compilation failed\n" << infoLog << std::endl;
		}

		id = glCreateProgram();
		glAttachShader(id, compute);
		glLinkProgram(id);

		//check again
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			std::cout << "shader linking failed\n" << infoLog << std::endl;
		}

		glDeleteShader(compute);

		loaded = true;
	}

	void load(const char* vertexPath, const char* fragmentPath) {
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			std::cout << "shader file(s) not successfully read" << std::endl;
		}

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		//first do vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		//check we didnt mess it up
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::cout << "vertex shader compilation failed\n" << infoLog << std::endl;
		}

		//next do the fragment shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		//again, check it compiled alright
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::cout << "fragment shader compilation failed\n" << infoLog << std::endl;
		}

		//now link them into a shader program
		id = glCreateProgram();
		glAttachShader(id, vertex);
		glAttachShader(id, fragment);
		glLinkProgram(id);

		//check that didnt go wrong
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			std::cout << "shader linking failed\n" << infoLog << std::endl;
		}

		//we dont need these anymore
		glDeleteShader(vertex);
		glDeleteShader(fragment);

		loaded = true;
	}

	//activate shader
	void use() {
		if (loaded) glUseProgram(id);
	}

	void setFloat(const char* name, float val) {
		glUniform1f(glGetUniformLocation(id, name), val);
	}

	void setInt(const char* name, int val) {
		glUniform1i(glGetUniformLocation(id, name), val);
	}
};