#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

void ReadShaderFile(const std::string& file, std::string& contents);

using namespace std;

namespace ES2 {

const int OffscreenCount = 5;
    
struct PVRTextureHeader {
    unsigned int HeaderSize;
    unsigned int Height;
    unsigned int Width;
};

struct Framebuffers {
    GLuint Backbuffer;
    GLuint Scene;
    GLuint Offscreen[OffscreenCount];
};

struct Renderbuffers {
    GLuint BackbufferColor;
    GLuint OffscreenColor[OffscreenCount];
    GLuint SceneColor;
    GLuint SceneDepth;
};

struct Textures {
    GLuint TombWindow;
    GLuint Scene;
    GLuint Offscreen[OffscreenCount];
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

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(float theta) const;
private:
    Drawable CreateDrawable(const ParametricSurface& surface, int flags) const;
    void RenderDrawable(const Drawable& drawable, const ProgramHandles& ) const;
    void RenderBackground() const;
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    void BuildProgram(const char* vs, const char* fs, ProgramHandles&) const;
    Textures m_textures;
    Drawable m_kleinBottle;
    Drawable m_quad;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    ProgramHandles m_blitting;
    ProgramHandles m_lighting;
    ivec2 m_size;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}
    
RenderingEngine::RenderingEngine()
{
    glGenRenderbuffers(1, &m_renderbuffers.BackbufferColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.BackbufferColor);
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

GLuint CreateFboTexture(int w, int h)
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

    // Create some geometry:
    m_kleinBottle = CreateDrawable(KleinBottle(0.2), VertexFlagsNormals);
    m_quad = CreateDrawable(Quad(2, 2), VertexFlagsTexCoords);
    
    // Extract width and height from the color buffer:
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_size.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_size.y);
    
    // Create the on-screen FBO:
    glGenFramebuffers(1, &m_framebuffers.Backbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Backbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.BackbufferColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.BackbufferColor);

    // Create the depth buffer for the full-size off-screen FBO:
    glGenRenderbuffers(1, &m_renderbuffers.SceneDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_size.x, m_size.y);
    glGenRenderbuffers(1, &m_renderbuffers.SceneColor);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneColor);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, m_size.x, m_size.y);
    glGenFramebuffers(1, &m_framebuffers.Scene);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Scene);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffers.SceneColor);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderbuffers.SceneDepth);
    m_textures.Scene = CreateFboTexture(m_size.x, m_size.y);
    
    // Create FBOs for the half, quarter, and eighth sizes:
    int w = m_size.x, h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        glGenRenderbuffers(1, &m_renderbuffers.OffscreenColor[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenColor[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, w, h);
        glGenFramebuffers(1, &m_framebuffers.Offscreen[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Offscreen[i]);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffers.OffscreenColor[i]);
        m_textures.Offscreen[i] = CreateFboTexture(w, h);
    }
    
    // Create the GLSL programs:
    BuildProgram("Blitting.es2.vert", "Blitting.es2.frag", m_blitting);
    BuildProgram("Lighting.es2.vert", "Lighting.es2.frag", m_lighting);

    // Set up some default material parameters.
    glUseProgram(m_lighting.Program);
    glUniform3f(m_lighting.Uniforms.DiffuseMaterial, 0.75f, 0.25f, 0);
    glUniform3f(m_lighting.Uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
    glUniform3f(m_lighting.Uniforms.SpecularMaterial, 0.5f, 0.5f, 0.5f);
    glUniform1f(m_lighting.Uniforms.Shininess, 50);
    
    // Set up the transforms:
    const float NearPlane = 5, FarPlane = 50;
    const float Scale = 0.004;
    const float HalfWidth = Scale * m_size.x / 2;
    const float HalfHeight = Scale * m_size.y / 2;
    
    mat4 projection = mat4::Frustum(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight,
                                    NearPlane, FarPlane);

    glUseProgram(m_lighting.Program);
    glUniformMatrix4fv(m_lighting.Uniforms.Projection, 1, 0, projection.Pointer());
    glBlendFunc(GL_ONE, GL_ONE);
}

void RenderingEngine::Render(float theta) const
{
    glViewport(0, 0, m_size.x, m_size.y);
    glEnable(GL_DEPTH_TEST);

    // Set the render target to the full size offscreen buffer.
    glBindTexture(GL_TEXTURE_2D, m_textures.TombWindow);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Scene);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.SceneColor);

    // Blit the background texture.
    glUseProgram(m_blitting.Program);
    glUniform1f(m_blitting.Uniforms.Threshold, 0);
    glDepthFunc(GL_ALWAYS);
    RenderDrawable(m_quad, m_blitting);

    // Set the light position.
    glUseProgram(m_lighting.Program);
    vec4 lightPosition(0.25, 0.25, 1, 0);
    glUniform3fv(m_lighting.Uniforms.LightPosition, 1, lightPosition.Pointer());

    // Set the model-view transform.
    const float distance = 10;
    const vec3 target(0, -0.15, 0);
    const vec3 up(0, 1, 0);
    const vec3 eye = vec3(0, 0, distance);
    const mat4 view = mat4::LookAt(eye, target, up);
    const mat4 model = mat4::RotateY(theta * 180.0f / 3.14f);
    const mat4 modelview = model * view;
    glUniformMatrix4fv(m_lighting.Uniforms.Modelview, 1, 0, modelview.Pointer());

    // Set the normal matrix.
    mat3 normalMatrix = modelview.ToMat3();
    glUniformMatrix3fv(m_lighting.Uniforms.NormalMatrix, 1, 0, normalMatrix.Pointer());

    // Render the Klein Bottle.
    glDepthFunc(GL_LESS);
    glEnableVertexAttribArray(m_lighting.Attributes.Normal);
    RenderDrawable(m_kleinBottle, m_lighting);
    
    // Set up OpenGL for rendering full-screen quads.
    glUseProgram(m_blitting.Program);
    glUniform1f(m_blitting.Uniforms.Threshold, 0.85);
    glDisable(GL_DEPTH_TEST);

    // Downsample the rendered scene.
    int w = m_size.x, h = m_size.y;
    for (int i = 0; i < OffscreenCount; ++i, w >>= 1, h >>= 1) {
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Offscreen[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenColor[i]);
        glBindTexture(GL_TEXTURE_2D, i ? m_textures.Offscreen[i - 1] : m_textures.Scene);
        RenderDrawable(m_quad, m_blitting);
        glUniform1f(m_blitting.Uniforms.Threshold, 0);
    }
    
    // Accumulate the downsampled buffers onto the backbuffer.
    glUniform1f(m_blitting.Uniforms.Threshold, 0);
    glViewport(0, 0, m_size.x, m_size.y);
    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Offscreen[0]);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.OffscreenColor[0]);
    for (int i = 0; i < OffscreenCount; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_textures.Offscreen[i]);
        RenderDrawable(m_quad, m_blitting);
    }

    // Blit the full-color buffer onto the backbuffer.
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Backbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.BackbufferColor);
    glBindTexture(GL_TEXTURE_2D, m_textures.Scene);
    RenderDrawable(m_quad, m_blitting);
    
    // Add the bloom texture onto the backbuffer.
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Offscreen[0]);
    RenderDrawable(m_quad, m_blitting);
    glDisable(GL_BLEND);
}

GLuint RenderingEngine::BuildShader(const char* filename, GLenum shaderType) const
{
    string contents;
    ReadShaderFile(filename, contents);
    const char* source = contents.c_str();
    
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

}