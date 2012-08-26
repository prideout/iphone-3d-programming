static const char* SimpleFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoordOut;

uniform sampler2D Sampler;
uniform mediump float AlphaTest;
uniform mediump vec3 OutlineColor;
uniform mediump vec3 GlyphColor;
uniform mediump vec3 GlowColor;

uniform bool Smooth;
uniform bool Outline;
uniform bool Glow;
uniform bool Shadow;

const mediump vec2 ShadowOffset = vec2(-0.005, 0.01);
const mediump vec3 ShadowColor = vec3(0.0, 0.0, 0.125);
const mediump float SmoothCenter = 0.5;
const mediump float OutlineCenter = 0.4;
const mediump float GlowCenter = 1.0;

void main(void)
{
    gl_FragColor = texture2D(Sampler, TextureCoordOut);

}
);
