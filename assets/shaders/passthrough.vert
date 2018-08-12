precision highp float;

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

out vec4 Color;
out vec2 TexCoord;

void main()
{
    gl_Position = ciModelViewProjection * ciPosition;
	Color = ciColor;
    TexCoord = ciTexCoord0;
}
