static const char* CubemapFragmentShader = STRINGIFY(

varying mediump vec3 ReflectDir;

uniform samplerCube Sampler;

void main(void)
{
    gl_FragColor = textureCube(Sampler, ReflectDir);
}
);
