#version 430 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 vertColor;

out float value;
out vec4 color;

uniform mat4 projection;

void main() {
	value = position.z; //collect that sneaky value from earlier
	gl_Position = projection * vec4(position.xy, 0, 1);
	color = vertColor;
}