#version 430 core
layout (location = 0) in vec2 vertex; //position

uniform mat4 model; //translation and scaling
//no need for view matrix as view never changes
uniform mat4 projection;

void main() {
	gl_Position = projection * model * vec4(vertex, 0.0, 1.0);
}