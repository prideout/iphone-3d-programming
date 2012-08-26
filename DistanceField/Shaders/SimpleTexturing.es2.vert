static const char* SimpleVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec2 TextureCoord;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat4 TextureMatrix;

varying vec2 TextureCoordOut;

void main(void)
{
    gl_Position = Projection * Modelview * Position;
    vec4 tc = TextureMatrix * vec4(TextureCoord, 0, 1);
    TextureCoordOut = tc.xy;
}
);
