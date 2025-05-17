#version 330 compatibility

out	vec4	varyingColor;

void main(void) {
	varyingColor	= gl_Color;
	gl_Position		= gl_Vertex + vec4(0.5, 0.5, 0.0, 0.0);
}