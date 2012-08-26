static const char* SimpleFragmentShader = STRINGIFY(

varying lowp vec4 DestinationColor;
varying mediump vec2 TextureCoordOut;

uniform sampler2D Sampler;

void main(void)
{
    gl_FragColor = texture2D(Sampler, TextureCoordOut) * DestinationColor;
}
);
