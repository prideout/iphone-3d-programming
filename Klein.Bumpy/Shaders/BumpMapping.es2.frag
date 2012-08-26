static const char* BumpFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoord;
varying mediump vec3 ObjectSpaceNormal;
varying mediump vec3 ObjectSpaceTangent;

uniform highp vec3 AmbientMaterial;
uniform highp vec3 DiffuseMaterial;
uniform highp vec3 SpecularMaterial;
uniform highp float Shininess;
uniform highp vec3 LightVector;
uniform highp vec3 EyeVector;

uniform sampler2D Sampler;

void main(void)
{
    // Extract the perturbed normal from the texture:
    highp vec3 tangentSpaceNormal = texture2D(Sampler, TextureCoord).yxz * 2.0 - 1.0;

    // Create a set of basis vectors:
    highp vec3 n = normalize(ObjectSpaceNormal);
    highp vec3 t = normalize(ObjectSpaceTangent);
    highp vec3 b = normalize(cross(n, t));

    // Change the perturbed normal from tangent space to object space:
    highp mat3 basis = mat3(n, t, b);
    highp vec3 N = basis * tangentSpaceNormal;
    
    // Perform standard lighting math:
    highp vec3 L = LightVector;
    highp vec3 E = EyeVector;
    highp vec3 H = normalize(L + E);
    highp float df = max(0.0, dot(N, L));
    highp float sf = max(0.0, dot(N, H));
    sf = pow(sf, Shininess);

    lowp vec3 color = AmbientMaterial + df * DiffuseMaterial + sf * SpecularMaterial;
    gl_FragColor = vec4(color, 1);
}
);
