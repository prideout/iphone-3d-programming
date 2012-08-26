static const char* HighPassFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoord;

uniform sampler2D Sampler;
uniform mediump float Threshold;

const mediump vec3 Perception = vec3(0.299, 0.587, 0.114);

void main(void)
{
    mediump vec3 color = texture2D(Sampler, TextureCoord).xyz;
    mediump float luminance = dot(Perception, color);
    gl_FragColor = (luminance > Threshold) ? vec4(color, 1) : vec4(0);
}
);
