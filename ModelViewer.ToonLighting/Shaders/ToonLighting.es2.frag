static const char* SimpleFragmentShader = STRINGIFY(

varying mediump vec3 EyespaceNormal;
varying lowp vec3 Diffuse;

uniform highp vec3 LightPosition;
uniform highp vec3 AmbientMaterial;
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

    if (df < 0.1) df = 0.0;
    else if (df < 0.3) df = 0.3;
    else if (df < 0.6) df = 0.6;
    else df = 1.0;
    
    sf = step(0.5, sf);

    lowp vec3 color = AmbientMaterial + df * Diffuse + sf * SpecularMaterial;

    gl_FragColor = vec4(color, 1);
}
);
