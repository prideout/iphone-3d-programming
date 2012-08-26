static const char* CubemapVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat3 Model;
uniform vec3  EyePosition;

varying vec3 ReflectDir;

void main(void)
{
    gl_Position = Projection * Modelview * Position;
    
    // Compute eye direction in object space:
    mediump vec3 eyeDir = normalize(Position.xyz - EyePosition);

    // Reflect eye direction over normal and transform to world space:
    ReflectDir = Model * reflect(eyeDir, Normal);
//	ReflectDir = Model * Position.xyz; // Keep this: it's a good way to test with unit sphere
}
);
