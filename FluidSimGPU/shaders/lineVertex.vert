#version 430 core
layout (location = 0) in vec2 vertex; //position

//no need for model matrix as line is defined in world coordinates
//no need for view matrix as view never changes
uniform mat4 projection;

void main() {
	gl_Position = projection * vec4(vertex, 0.0, 1.0);
}