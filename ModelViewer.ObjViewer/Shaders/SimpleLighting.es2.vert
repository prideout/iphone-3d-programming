static const char* SimpleVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec3 Normal;
attribute vec3 DiffuseMaterial;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat3 NormalMatrix;
uniform vec3 LightPosition;
uniform vec3 AmbientMaterial;
uniform vec3 SpecularMaterial;
uniform float Shininess;

varying vec4 DestinationColor;

void main(void)
{
    vec3 N = NormalMatrix * Normal;
    vec3 L = normalize(LightPosition);
    vec3 E = vec3(0, 0, 1);
    vec3 H = normalize(L + E);

    float df = max(0.0, dot(N, L));
    float sf = max(0.0, dot(N, H));
    sf = pow(sf, Shininess);

    vec3 color = AmbientMaterial + df * DiffuseMaterial + sf * SpecularMaterial;
    
    DestinationColor = vec4(color, 1);
    gl_Position = Projection * Modelview * Position;
}
);
