static const char* SimpleVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;
attribute vec3 DiffuseMaterial;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat3 NormalMatrix;

varying vec3 EyespaceNormal;
varying vec3 Diffuse;

void main(void)
{
    EyespaceNormal = NormalMatrix * Normal;
    Diffuse = DiffuseMaterial;
    gl_Position = Projection * Modelview * Position;
}
);
