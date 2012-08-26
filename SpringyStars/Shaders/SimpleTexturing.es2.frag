static const char* SimpleFragmentShader = STRINGIFY(

varying mediump vec2 TextureCoordOut;

uniform sampler2D Sampler;
uniform bool IsSprite;

void main(void)
{
    gl_FragColor = texture2D(Sampler, IsSprite ? gl_PointCoord : TextureCoordOut);
}
);
