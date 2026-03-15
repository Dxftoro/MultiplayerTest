#version 430

layout(location = 0) in vec2 aPosition;

out vec3 oColor;

uniform mat4 uProjectionMatrix;
uniform vec3 uPosition;
uniform vec3 uColor;

void main() {
	vec3 wPosition = vec3(aPosition * 20.0, 0.0) + uPosition;
	oColor = uColor;
	gl_Position = uProjectionMatrix * vec4(wPosition, 1.0);
}