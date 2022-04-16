#version 430 core
out vec4 FragColor;

in float value;
in vec4 color;

void main() {
	//this creates a brighter color on the edge of the fluid
	if (value < 2) FragColor = mix(vec4(1,1,1,min(1, 1.5 * color.a)), color, value / 3);
	else FragColor = color;
}