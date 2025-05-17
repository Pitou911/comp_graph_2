#version 330 compatibility

in	vec4	varyingColor;
out	vec4	outColor;

void main(void) {
	outColor = varyingColor;
}