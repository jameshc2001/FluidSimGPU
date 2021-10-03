#version 430 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 vertColor;

out vec4 color;

uniform mat4 projection;

void main() {
	gl_Position = projection * position;
	color = vertColor;
}