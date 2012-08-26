static const char* BumpFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoord;
varying mediump vec3 ObjectSpaceNormal;
varying mediump vec3 ObjectSpaceTangent;

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
    
    // Transform the normal from unit space to color space:
    gl_FragColor = vec4((N + 1.0) * 0.5, 1);
}
);
