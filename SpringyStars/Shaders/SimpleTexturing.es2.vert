static const char* SimpleVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec2 TextureCoord;

uniform mat4 Projection;
uniform mat4 Modelview;

varying vec2 TextureCoordOut;

void main(void)
{
    gl_Position = Projection * Modelview * Position;
    TextureCoordOut = TextureCoord;
    gl_PointSize = 15.0;
}
);
