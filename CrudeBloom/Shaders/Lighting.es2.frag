varying mediump vec3 EyespaceNormal;

uniform highp vec3 LightPosition;
uniform highp vec3 AmbientMaterial;
uniform highp vec3 DiffuseMaterial;
uniform highp vec3 SpecularMaterial;
uniform highp float Shininess;

void main(void)
{
    highp vec3 N = normalize(EyespaceNormal);
    highp vec3 L = normalize(LightPosition);
    highp vec3 E = vec3(0, 0, 1);
    highp vec3 H = normalize(L + E);
    
    highp float df = max(0.0, dot(N, L));
    highp float sf = max(0.0, dot(N, H));
    sf = pow(sf, Shininess);

    lowp vec3 color = AmbientMaterial + df * DiffuseMaterial + sf * SpecularMaterial;

    gl_FragColor = vec4(color, 1);
}
