#version 430

layout(location = 0) in vec2 aPosition;

uniform mat4 uProjectionMatrix;
uniform vec3 uPosition;

void main() {
	vec3 wPosition = vec3(aPosition * 20.0, 0.0) + uPosition;
	gl_Position = uProjectionMatrix * vec4(wPosition, 1.0);
}