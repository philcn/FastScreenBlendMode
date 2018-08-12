uniform sampler2D uInputTexture;

in vec4 Color;
in vec2 TexCoord;

out vec4 oColor;

void main()
{
	vec4 textureSample = texture(uInputTexture, TexCoord);
	oColor = vec4(1.0) - textureSample * Color.a;
}
