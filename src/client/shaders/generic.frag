#version 430

in vec3 oColor;

void main() {
	gl_FragColor = vec4(oColor, 0.0);
}