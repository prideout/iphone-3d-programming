static const char* GaussianFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoord;

uniform sampler2D Sampler;
uniform mediump float Coefficients[3];
uniform mediump vec2 Offset;

void main(void)
{
    mediump vec3 A = Coefficients[0] * texture2D(Sampler, TextureCoord - Offset).xyz;
    mediump vec3 B = Coefficients[1] * texture2D(Sampler, TextureCoord).xyz;
    mediump vec3 C = Coefficients[2] * texture2D(Sampler, TextureCoord + Offset).xyz;
    mediump vec3 color = A + B + C;
    gl_FragColor = vec4(color, 1);
}
);
