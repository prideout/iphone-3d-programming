static const char* LightingVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat3 NormalMatrix;

varying vec3 EyespaceNormal;
varying vec3 Diffuse;

void main(void)
{
    EyespaceNormal = NormalMatrix * Normal;
    gl_Position = Projection * Modelview * Position;
}
);
