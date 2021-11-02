#version 430 core
out vec4 FragColor;

in float value;
in vec4 color;

void main() {
	//do cool stuff to colour maybe?
	if (value < 2) FragColor = mix(vec4(1,1,1,color.a), color, value / 3);
	else FragColor = color;
	//FragColor = mix(vec4(1,1,1,color.a), color, value / 3);
}