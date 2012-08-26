attribute vec4 Position;
attribute vec2 TextureCoordIn;

varying vec2 TextureCoord;

void main(void)
{
    gl_Position = Position;
    gl_Position.z = 1.0;
    TextureCoord = TextureCoordIn;
}
