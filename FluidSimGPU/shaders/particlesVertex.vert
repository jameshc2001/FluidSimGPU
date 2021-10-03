#version 430 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 vertColor;

out vec3 color;

uniform mat4 projection;

void main() {
	gl_Position = projection * vec4(position, 0.0, 1.0);
	color = vertColor;
}