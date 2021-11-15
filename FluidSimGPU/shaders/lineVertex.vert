#version 430 core
layout (location = 0) in vec2 vertex; //position
layout (location = 1) in vec3 vertColor;

//no need for model matrix as line is defined in world coordinates
//no need for view matrix as view never changes
uniform mat4 projection;
out vec3 color;

void main() {
	gl_Position = projection * vec4(vertex, 0.0, 1.0);
	color = vertColor;
}