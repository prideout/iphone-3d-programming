static const char* SunVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec2 TextureCoordIn;

uniform mat4 Projection;
uniform mat4 Modelview;

varying vec2 TextureCoord;

void main(void)
{
    gl_Position = Projection * Modelview * Position;
    TextureCoord = TextureCoordIn;
}
);
