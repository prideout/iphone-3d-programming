#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/Cubemap.es2.vert"
#include "../Shaders/Cubemap.es2.frag"
#include "../Shaders/Simple.es2.vert"
#include "../Shaders/Simple.es2.frag"

using namespace std;

namespace ES2 {

struct PVRTextureHeader {
    unsigned int HeaderSize;
    unsigned int Height;
    unsigned int Width;
};

struct Framebuffers {
    GLuint Screen;
};

struct Renderbuffers {
    GLuint Color;
    GLuint Depth;
};

struct Textures {
    GLuint Metal;
    GLuint Cubemap;
};

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
    int Flags;
};

struct UniformHandles {
    GLint Modelview;
    GLint Projection;
    GLint Sampler;
    GLint Model;
    GLint EyePosition;
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
    ProgramHandles m_simple;
    ProgramHandles m_cubemap;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}
    
RenderingEngine::RenderingEngine()
{
    glGenRenderbuffers(1, &m_renderbuffers.Color);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Color);
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

GLuint CreateCubemap(GLvoid** faceData, int size, GLenum format, GLenum type)
{
    GLuint textureObject;
    glGenTextures(1, &textureObject);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureObject);
    for (int f = 0; f < 6; ++f) {
        GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + f;
        glTexImage2D(face, 0, format, size, size, 0, format, type, faceData[f]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, 
                    GL_TEXTURE_MIN_FILTER, 
                    GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    return textureObject;
}
    
void RenderingEngine::Initialize()
{
    const unsigned int* faces[] = { Face0, Face1, Face2, Face3, Face4, Face5 };

    // Load the background texture:
    glGenTextures(1, &m_textures.Metal);
    glBindTexture(GL_TEXTURE_2D, m_textures.Metal);
    {
        PVRTextureHeader* header = (PVRTextureHeader*) Metal;
        GLsizei w = header->Width;
        GLsizei h = header->Height;
        const unsigned int* texels = Metal + header->HeaderSize / 4;
        GLenum format = GL_RGB;
        GLenum type = GL_UNSIGNED_SHORT_5_6_5;
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, texels);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load the cubemap texture:
    PVRTextureHeader* header = (PVRTextureHeader*) faces[0];
    const unsigned int* faceData[] = {
        faces[0] + header->HeaderSize / 4,
        faces[1] + header->HeaderSize / 4,
        faces[2] + header->HeaderSize / 4,
        faces[3] + header->HeaderSize / 4,
        faces[4] + header->HeaderSize / 4,
        faces[5] + header->HeaderSize / 4 };

    m_textures.Cubemap = CreateCubemap((void**) faceData, header->Width, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);

    // Create some geometry:
    m_kleinBottle = CreateDrawable(KleinBottle(0.2), VertexFlagsNormals);
    //m_kleinBottle = CreateDrawable(Sphere(1), VertexFlagsNormals);
    m_quad = CreateDrawable(Quad(6, 9), VertexFlagsTexCoords);
    
    // Extract width and height from the color buffer:
    ivec2 screenSize;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_WIDTH, &screenSize.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_HEIGHT, &screenSize.y);
    
    // Create the depth buffer:
    glGenRenderbuffers(1, &m_renderbuffers.Depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                             screenSize.x, screenSize.y);
    
    // Create the on-screen FBO:
    glGenFramebuffers(1, &m_framebuffers.Screen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Screen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.Color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_renderbuffers.Depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Color);
    
    // Create the GLSL programs:
    BuildProgram(SimpleVertexShader, SimpleFragmentShader, m_simple);
    BuildProgram(CubemapVertexShader, CubemapFragmentShader, m_cubemap);
    
    // Set up various GL state:
    glViewport(0, 0, screenSize.x, screenSize.y);
    glEnable(GL_DEPTH_TEST);

    // Set up the transforms:
    const float NearPlane = 5, FarPlane = 50;
    const float Scale = 0.004;
    const float HalfWidth = Scale * screenSize.x / 2;
    const float HalfHeight = Scale * screenSize.y / 2;
    
    mat4 projection = mat4::Frustum(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight,
                                    NearPlane, FarPlane);

    glUseProgram(m_cubemap.Program);
    glUniformMatrix4fv(m_cubemap.Uniforms.Projection, 1, 0, projection.Pointer());
    
    glUseProgram(m_simple.Program);
    glUniformMatrix4fv(m_simple.Uniforms.Projection, 1, 0, projection.Pointer());
}

void RenderingEngine::Render(float theta) const
{
    const float distance = 10;
    const vec3 target(0, -0.15, 0);
    const vec3 up(0, 1, 0);

    vec3 eye(0, -0.15, distance * 2);
    mat4 view = mat4::LookAt(eye, target, up);
    
    glUseProgram(m_simple.Program);
    glUniformMatrix4fv(m_simple.Uniforms.Modelview, 1, 0, view.Pointer());
    glDepthFunc(GL_ALWAYS);
    glBindTexture(GL_TEXTURE_2D, m_textures.Metal);
    
    RenderDrawable(m_quad, m_simple);
        
    eye = vec3(0, 0, distance);
    view = mat4::LookAt(eye, target, up);
    
    const mat4 model = mat4::RotateY(theta * 180.0f / 3.14f);
    const mat3 model3x3 = model.ToMat3();
    const mat4 modelview = model * view;

    vec4 eyeWorldSpace(0, 0, -10, 1);
    vec4 eyeObjectSpace = model * eyeWorldSpace;

    glUseProgram(m_cubemap.Program);
    glUniform3fv(m_cubemap.Uniforms.EyePosition, 1, eyeObjectSpace.Pointer());
    glUniformMatrix4fv(m_cubemap.Uniforms.Modelview, 1, 0, modelview.Pointer());
    glUniformMatrix3fv(m_cubemap.Uniforms.Model, 1, 0, model3x3.Pointer());
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textures.Cubemap);
    glEnableVertexAttribArray(m_cubemap.Attributes.Normal);
    glDepthFunc(GL_LESS);
    
    RenderDrawable(m_kleinBottle, m_cubemap);
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
    uniforms.Projection = glGetUniformLocation(programHandle,  "Projection");
    uniforms.Modelview = glGetUniformLocation(programHandle,  "Modelview");
    uniforms.Sampler = glGetUniformLocation(programHandle,  "Sampler");
    uniforms.Model = glGetUniformLocation(programHandle,  "Model");
    uniforms.EyePosition = glGetUniformLocation(programHandle,  "EyePosition");
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