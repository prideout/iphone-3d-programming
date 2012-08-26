const int BoneCount = 17;

attribute vec4 Position;
attribute vec2 TextureCoordIn;
attribute vec2 BoneWeights;
attribute vec2 BoneIndices;

uniform mat4 Projection;
uniform mat4 Modelview[BoneCount];

varying vec2 TextureCoord;

void main(void)
{
    vec4 p0 = Modelview[int(BoneIndices.x)] * Position;
    vec4 p1 = Modelview[int(BoneIndices.y)] * Position;
    vec4 p = p0 * BoneWeights.x + p1 * BoneWeights.y;
    gl_Position = Projection * p;
    TextureCoord = TextureCoordIn;
}
