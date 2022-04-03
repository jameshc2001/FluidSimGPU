#version 430 core
out vec4 FragColor;

uniform vec2 circleCentre;
uniform float circleRadius;

void main() {
	vec2 coord = gl_FragCoord.xy - circleCentre;
	float circle = step(length(coord), circleRadius);
	FragColor = vec4(circle, 0, 0, circle/2);
}