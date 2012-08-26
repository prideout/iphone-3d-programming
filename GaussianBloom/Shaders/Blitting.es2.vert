static const char* BlittingVertexShader = STRINGIFY(

attribute vec4 Position;
attribute vec2 TextureCoordIn;

varying vec2 TextureCoord;

void main(void)
{
    gl_Position = Position;
    TextureCoord = TextureCoordIn;
}
);
