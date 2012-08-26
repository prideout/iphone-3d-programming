#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/Blitting.es2.vert"
#include "../Shaders/Blitting.es2.frag"
#include "../Shaders/Lighting.es2.vert"
#include "../Shaders/Lighting.es2.frag"
#include "../Shaders/Gaussian.es2.frag"
#include "../Shaders/HighPass.es2.frag"
#include "../Shaders/Sun.es2.vert"

using namespace std;

namespace ES2 {

struct PVRTextureHeader {
    unsigned int HeaderSize;
    unsigned int Height;
    unsigned int Width;
};

const int OffscreenCount = 5;
const bool Optimize = true;
    
struct Framebuffers {
    GLuint Onscreen;
    GLuint Scene;
    GLuint OffscreenLeft[OffscreenCount];
    GLuint OffscreenRight[OffscreenCount];
};

struct Renderbuffers {
    GLuint Onscreen;
    GLuint OffscreenLeft[OffscreenCount];
    GLuint OffscreenRight[OffscreenCount];
    GLuint SceneColor;
    GLuint SceneDepth;
};

struct Textures {
    GLuint TombWindow;
    GLuint Sun;
    GLuint Scene;
    GLuint OffscreenLeft[OffscreenCount];
    GLuint OffscreenRight[OffscreenCount];
};

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
    int Flags;
};

struct UniformHandles {
    GLint Projection;
    GLint Modelview;
    GLint NormalMatrix;
    GLint LightPosition;
    GLint AmbientMaterial;
    GLint DiffuseMaterial;
    GLint SpecularMaterial;
    GLint Shininess;
    GLint Threshold;
    GLint Coefficients;
    GLint Offset;
};

struct AttributeHandles {
    GLint Position;
    GLint Normal;
    GLint TexCoord;
};

struct ProgramHandles {
    GLuint Program;
    AttributeHandles Attributes;
    UniformHandles Uniforms;
};

struct Programs {
    ProgramHandles Blit;
    ProgramHandles Light;
    ProgramHandles Blur;
    ProgramHandles HighPass;
    ProgramHandles Sun;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(float theta) const;
    void SetSunPosition(vec3 sunPosition);
private:
    Drawable CreateDrawable(const ParametricSurface& surface, int flags) const;
    void RenderDrawable(const Drawable& drawable, const ProgramHandles& ) const;
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    void BuildProgram(const char* vs, const char* fs, ProgramHandles&) const;
    GLuint CreateFboTexture(int w, int h) const;
    Textures m_textures;
    Drawable m_kleinBottle;
    Drawable m_quad;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    Programs m_programs;
    ivec2 m_size;
    vec3 m_sunPosition;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}
    
RenderingEngine::RenderingEngine()
{
    glGenRenderbuffers(1, &m_renderbuffers.Onscreen);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Onscreen);
    
    m_sunPosition = vec3(-3.5, 5, 0);
}

void PrettyPrintExtensions()
{
    string extensions = (const char*) glGetString(GL_EXTENSIONS);
    char* extensionStart = &extensions[0];
    char** extension = &extensionStart;
    cout << "Supported OpenGL ES Extensions:" << endl;
    while (*extension)
        cout << '\t' << strsep(extension, " ") << endl;
    cout << endl;
}

GLuint RenderingEngine::CreateFboTexture(int w, int h) const
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    return texture;
}

void RenderingEngine::Initialize()
{
    // Load the background texture:
    glGenTextures(1, &m_textures.TombWindow);
    glBindTexture(GL_TEXTURE_2D, m_textures.TombWindow);
    {
        PVRTextureHeader* header = (PVRTextureHeader*) TombWindow;
        GLsizei w = header->Width;
        GLsizei h = header->Height;
        const unsigned int* texels = TombWindow + header->HeaderSize / 4;
        GLenum format = GL_RGB;
        GLenum type = GL_UNSIGNED_SHORT_5_6_5;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, texels);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load the sun texture:
    glGenTextures(1, &m_textures.Sun);
    glBindTexture(GL_TEXTURE_2D, m_textures.Sun);
    {
        PVRTextureHeader* header = (PVRTextureHeader*) Circle;
        GLsizei w = header->Width;
        GLsizei h = header->Height;
        const unsigned int* texels = Circle + header->HeaderSize / 4;
        GLenum format = GL_LUMINANCE;
        GLenum type = GL_UNSIGNED_BYTE;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, texels);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create some geometry:
    m_kleinBottle = CreateDrawable(KleinBottle(0.2), VertexFlagsNormals);
    m_quad = CreateDrawable(Quad(2, 2), VertexFlagsTexCoords);
    
    // Extract width and height from the color buffer:
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_size.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_size.y);
    
    // Create the on-screen FBO:
    glGenFramebuffers(1, &m_framebuffers.Onscreen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Onscreen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.Onscreen);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Onscreen);

    // Create the depth buffer for the full-size off-screen FBO:
    glGenRenderbuffers(1, &m_renderbuffers.SceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_size.x, m_size.y);
    glGenRenderbuffers(1, &m_renderbuffers.SceneColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneColor);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, m_size.x, m_size.y);
    glGenFramebuffers(1, &m_framebuffers.Scene);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Scene);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.SceneColor);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_renderbuffers.SceneDepth);
    m_textures.Scene = CreateFboTexture(m_size.x, m_size.y);
    
    // Create FBOs for the half, quarter, and eighth sizes:
    int w = m_size.x, h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        glGenRenderbuffers(1, &m_renderbuffers.OffscreenLeft[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenLeft[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, w, h);
        glGenFramebuffers(1, &m_framebuffers.OffscreenLeft[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.OffscreenLeft[i]);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, m_renderbuffers.OffscreenLeft[i]);
        m_textures.OffscreenLeft[i] = CreateFboTexture(w, h);

        glGenRenderbuffers(1, &m_renderbuffers.OffscreenRight[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenRight[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, w, h);
        glGenFramebuffers(1, &m_framebuffers.OffscreenRight[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.OffscreenRight[i]);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffers.OffscreenRight[i]);
        m_textures.OffscreenRight[i] = CreateFboTexture(w, h);
    }
    
    // Create the GLSL programs:
    BuildProgram(BlittingVertexShader, BlittingFragmentShader, m_programs.Blit);
    BuildProgram(BlittingVertexShader, GaussianFragmentShader, m_programs.Blur);
    BuildProgram(LightingVertexShader, LightingFragmentShader, m_programs.Light);
    BuildProgram(BlittingVertexShader, HighPassFragmentShader, m_programs.HighPass);
    BuildProgram(SunVertexShader, BlittingFragmentShader, m_programs.Sun);

    // Set up some default material parameters.
    glUseProgram(m_programs.Light.Program);
    glUniform3f(m_programs.Light.Uniforms.DiffuseMaterial, 0.75f, 0.25f, 0);
    glUniform3f(m_programs.Light.Uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
    glUniform3f(m_programs.Light.Uniforms.SpecularMaterial, 0.6f, 0.5f, 0.5f);
    glUniform1f(m_programs.Light.Uniforms.Shininess, 50);
    
    // Set up the transforms:
    const float NearPlane = 5, FarPlane = 50;
    const float Scale = 0.004;
    const float HalfWidth = Scale * m_size.x / 2;
    const float HalfHeight = Scale * m_size.y / 2;
    
    mat4 projection = mat4::Frustum(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight,
                                    NearPlane, FarPlane);

    glUseProgram(m_programs.Light.Program);
    glUniformMatrix4fv(m_programs.Light.Uniforms.Projection, 1, 0, projection.Pointer());

    glUseProgram(m_programs.Sun.Program);
    glUniformMatrix4fv(m_programs.Sun.Uniforms.Projection, 1, 0, projection.Pointer());
}

void RenderingEngine::Render(float theta) const
{
    glViewport(0, 0, m_size.x, m_size.y);
    glEnable(GL_DEPTH_TEST);

    // Set the render target to the full size offscreen buffer:
    glBindTexture(GL_TEXTURE_2D, m_textures.TombWindow);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Scene);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneColor);

    // Blit the background texture:
    glUseProgram(m_programs.Blit.Program);
    glDepthFunc(GL_ALWAYS);
    RenderDrawable(m_quad, m_programs.Blit);

    // Draw the sun:
    {
        const float distance = 30;
        const vec3 target(0, -0.15, 0);
        const vec3 up(0, 1, 0);
        const vec3 eye = vec3(0, 0, distance);
        const mat4 view = mat4::LookAt(eye, target, up);
        const mat4 model = mat4::Translate(m_sunPosition.x, m_sunPosition.y, m_sunPosition.z);
        const mat4 modelview = model * view;

        glUseProgram(m_programs.Sun.Program);
        glUniformMatrix4fv(m_programs.Sun.Uniforms.Modelview, 1, 0, modelview.Pointer());

        glBlendFunc(GL_ONE, GL_ONE);
        glEnable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, m_textures.Sun);
        RenderDrawable(m_quad, m_programs.Blit);
        glDisable(GL_BLEND);
    }

    // Set the light position:
    glUseProgram(m_programs.Light.Program);
    vec4 lightPosition(0.25, 0.25, 1, 0);
    glUniform3fv(m_programs.Light.Uniforms.LightPosition, 1, lightPosition.Pointer());

    // Set the model-view transform:
    const float distance = 10;
    const vec3 target(0, -0.15, 0);
    const vec3 up(0, 1, 0);
    const vec3 eye = vec3(0, 0, distance);
    const mat4 view = mat4::LookAt(eye, target, up);
    const mat4 model = mat4::RotateY(theta * 180.0f / 3.14f);
    const mat4 modelview = model * view;
    glUniformMatrix4fv(m_programs.Light.Uniforms.Modelview, 1, 0, modelview.Pointer());

    // Set the normal matrix:
    mat3 normalMatrix = modelview.ToMat3();
    glUniformMatrix3fv(m_programs.Light.Uniforms.NormalMatrix, 1, 0, normalMatrix.Pointer());

    // Render the Klein Bottle:
    glDepthFunc(GL_LESS);
    glEnableVertexAttribArray(m_programs.Light.Attributes.Normal);
    RenderDrawable(m_kleinBottle, m_programs.Light);
    
    // Set up the high-pass filter:
    glUseProgram(m_programs.HighPass.Program);
    glUniform1f(m_programs.HighPass.Uniforms.Threshold, 0.85);
    glDisable(GL_DEPTH_TEST);

    // Downsample the rendered scene:
    int w = m_size.x, h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.OffscreenLeft[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenLeft[i]);
        glBindTexture(GL_TEXTURE_2D, i ? m_textures.OffscreenLeft[i - 1] :
                                         m_textures.Scene);
        if (i == 0) {
            RenderDrawable(m_quad, m_programs.HighPass);
            glUseProgram(m_programs.Blit.Program);
        } else {
            RenderDrawable(m_quad, m_programs.Blit);
        }
    }
    
    // Set up for gaussian blur:
    float kernel[3] = { 5.0f / 16.0f, 6 / 16.0f, 5 / 16.0f };
    glUseProgram(m_programs.Blur.Program);
    glUniform1fv(m_programs.Blur.Uniforms.Coefficients, 3, kernel);

    // Perform the horizontal blurring pass:
    w = m_size.x; h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        if (Optimize && i < 2)
            continue;
        float offset = 1.2f / (float) w;
        glUniform2f(m_programs.Blur.Uniforms.Offset, offset, 0);
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.OffscreenRight[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenRight[i]);
        glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenLeft[i]);
        RenderDrawable(m_quad, m_programs.Blur);
    }

    // Perform the vertical blurring pass:
    w = m_size.x; h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        if (Optimize && i < 2)
            continue;
        float offset = 1.2f / (float) h;
        glUniform2f(m_programs.Blur.Uniforms.Offset, 0, offset);
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.OffscreenLeft[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenLeft[i]);
        glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenRight[i]);
        RenderDrawable(m_quad, m_programs.Blur);
    }

    // Blit the full-color buffer onto the screen:
    glUseProgram(m_programs.Blit.Program);
    glViewport(0, 0, m_size.x, m_size.y);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Onscreen);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Onscreen);
    glBindTexture(GL_TEXTURE_2D, m_textures.Scene);
    RenderDrawable(m_quad, m_programs.Blit);

    // Accumulate the bloom textures onto the screen:
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    for (int i = 1; i < OffscreenCount; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenLeft[i]);
        RenderDrawable(m_quad, m_programs.Blit);
    }
    glDisable(GL_BLEND);
}

GLuint RenderingEngine::BuildShader(const char* source, GLenum shaderType) const
{
    GLuint shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
    
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    
    return shaderHandle;
}

void RenderingEngine::BuildProgram(const char* vertexShaderSource,
                                   const char* fragmentShaderSource,
                                   ProgramHandles& program) const
{
    GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram(programHandle);
    
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    
    program.Program = programHandle;

    AttributeHandles& attribs = program.Attributes;
    attribs.Position = glGetAttribLocation(programHandle, "Position");
    attribs.Normal = glGetAttribLocation(programHandle, "Normal");
    attribs.TexCoord = glGetAttribLocation(programHandle, "TextureCoordIn");
    
    UniformHandles& uniforms = program.Uniforms;
    uniforms.Projection = glGetUniformLocation(programHandle, "Projection");
    uniforms.Modelview = glGetUniformLocation(programHandle, "Modelview");
    uniforms.NormalMatrix = glGetUniformLocation(programHandle, "NormalMatrix");
    uniforms.LightPosition = glGetUniformLocation(programHandle, "LightPosition");
    uniforms.AmbientMaterial = glGetUniformLocation(programHandle, "AmbientMaterial");
    uniforms.DiffuseMaterial = glGetUniformLocation(programHandle, "DiffuseMaterial");
    uniforms.SpecularMaterial = glGetUniformLocation(programHandle, "SpecularMaterial");
    uniforms.Shininess = glGetUniformLocation(programHandle, "Shininess");
    uniforms.Threshold = glGetUniformLocation(programHandle, "Threshold");
    uniforms.Coefficients = glGetUniformLocation(programHandle, "Coefficients");
    uniforms.Offset = glGetUniformLocation(programHandle, "Offset");
}

void RenderingEngine::RenderDrawable(const Drawable& drawable,
                                     const ProgramHandles& program) const
{
    int stride = sizeof(vec3);

    if (drawable.Flags & VertexFlagsTexCoords)
        stride += sizeof(vec2);

    if (drawable.Flags & VertexFlagsNormals)
        stride += sizeof(vec3);
    
    GLvoid* offset = (GLvoid*) sizeof(vec3);

    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    
    const AttributeHandles& a = program.Attributes;
    
    glEnableVertexAttribArray(a.Position);
    glVertexAttribPointer(a.Position, 3, GL_FLOAT, GL_FALSE, stride, 0);
    
    if (drawable.Flags & VertexFlagsTexCoords) {
        glEnableVertexAttribArray(a.TexCoord);
        glVertexAttribPointer(a.TexCoord, 2, GL_FLOAT, GL_FALSE, stride, offset);
    }

    if (drawable.Flags & VertexFlagsNormals) {
        glEnableVertexAttribArray(a.Normal);
        glVertexAttribPointer(a.Normal, 3, GL_FLOAT, GL_FALSE, stride, offset);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    
    glDisableVertexAttribArray(a.Position);
    glDisableVertexAttribArray(a.TexCoord);
    glDisableVertexAttribArray(a.Normal);
}

Drawable RenderingEngine::CreateDrawable(const ParametricSurface& surface, int flags) const
{
    // Create the VBO for the vertices.
    VertexList vertices;
    surface.GenerateVertices(vertices, flags);
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(vertices[0]),
                 &vertices[0],
                 GL_STATIC_DRAW);
    
    // Create a new VBO for the indices if needed.
    int indexCount = surface.GetTriangleIndexCount();
    GLuint indexBuffer;
    IndexList indices(indexCount);
    surface.GenerateTriangleIndices(indices);
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indexCount * sizeof(GLushort),
                 &indices[0],
                 GL_STATIC_DRAW);
    
    // Fill in a descriptive struct and return it.
    Drawable drawable;
    drawable.IndexBuffer = indexBuffer;
    drawable.VertexBuffer = vertexBuffer;
    drawable.IndexCount = indexCount;
    drawable.Flags = flags;
    return drawable;
}

void RenderingEngine::SetSunPosition(vec3 sunPosition)
{
    m_sunPosition = sunPosition;
}

}