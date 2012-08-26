static const char* SimpleVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;
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
