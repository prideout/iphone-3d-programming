static const char* SimpleFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoord;

uniform sampler2D Sampler;

void main(void)
{
    lowp vec3 texel = texture2D(Sampler, TextureCoord).xyz;
    gl_FragColor = vec4(texel, 1);
}
);
