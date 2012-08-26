static const char* BumpVertexShader = STRINGIFY(

attribute vec3 Normal;
attribute vec3 Tangent;
attribute vec2 TextureCoordIn;

uniform mat4 Projection;

varying vec2 TextureCoord;
varying vec3 ObjectSpaceNormal;
varying vec3 ObjectSpaceTangent;

const float Distance = 10.0;
const vec2 Offset = vec2(0.5, 0.5);
const vec2 Scale = vec2(2.0, 4.0);

void main(void)
{
    ObjectSpaceNormal = Normal;
    ObjectSpaceTangent = Tangent;

    vec4 v = vec4(TextureCoordIn - Offset, -Distance, 1);
    gl_Position = Projection * v;
    gl_Position.xy *= Scale;
    
    TextureCoord = TextureCoordIn;
}
);
