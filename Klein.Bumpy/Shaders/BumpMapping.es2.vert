static const char* BumpVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;
attribute vec3 Tangent;
attribute vec2 TextureCoordIn;

uniform mat4 Projection;
uniform mat4 Modelview;

varying vec2 TextureCoord;
varying vec3 ObjectSpaceNormal;
varying vec3 ObjectSpaceTangent;

void main(void)
{
    ObjectSpaceNormal = Normal;
    ObjectSpaceTangent = Tangent;
    gl_Position = Projection * Modelview * Position;
    TextureCoord = TextureCoordIn;
}
);
