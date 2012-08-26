varying mediump vec2 TextureCoord;
uniform sampler2D Sampler;

void main(void)
{
    gl_FragColor = texture2D(Sampler, TextureCoord);
}
