#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/SimpleTexturing.es2.vert"
#include "../Shaders/SimpleTexturing.es2.frag"
#include "../Shaders/BumpMapping.es2.vert"
#include "../Shaders/BumpMapping.es2.frag"

using namespace std;

namespace ES2 {

#include "RenderingEngine.Common.hpp"

struct UniformHandles {
    GLint Modelview;
    GLint Projection;
    GLint LightVector;
    GLint EyeVector;
    GLint AmbientMaterial;
    GLint DiffuseMaterial;
    GLint SpecularMaterial;
    GLint Shininess;
    GLint Sampler;
};
    
struct AttributeHandles {
    GLint Position;
    GLint TextureCoord;
    GLint Normal;
    GLint Tangent;
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
    void RenderDrawable(const Drawable& drawable, const ProgramHandles& program) const;
    void RenderBackground() const;
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    void BuildProgram(const char* vs, const char* fs, ProgramHandles&) const;
    Textures m_textures;
    Drawable m_kleinBottle;
    Drawable m_quad;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    ProgramHandles m_simple;
    ProgramHandles m_bump;
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
    
void RenderingEngine::Initialize()
{
    // Load up some textures and VBOs:
    m_textures.Tiger = CreateTexture(TigerTexture);

    m_textures.ObjectSpaceNormals = CreateTexture(ObjectSpaceNormals);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glGenerateMipmap(GL_TEXTURE_2D);

    m_textures.TangentSpaceNormals = CreateTexture(TangentSpaceNormals);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glGenerateMipmap(GL_TEXTURE_2D);

    m_kleinBottle = CreateDrawable(KleinBottle(0.2), VertexFlagsTexCoords | VertexFlagsNormals | VertexFlagsTangents);
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
    BuildProgram(BumpVertexShader, BumpFragmentShader, m_bump);
    
    // Set up some default material parameters:
    glUseProgram(m_bump.Program);
    glUniform3f(m_bump.Uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
    glUniform3f(m_bump.Uniforms.DiffuseMaterial, 0.75f, 0.75f, 0.75f);
    glUniform3f(m_bump.Uniforms.SpecularMaterial, 0.5, 0.5, 0.5);
    glUniform1f(m_bump.Uniforms.Shininess, 50);
    
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

    glUseProgram(m_bump.Program);
    glUniformMatrix4fv(m_bump.Uniforms.Projection, 1, 0, projection.Pointer());
    
    glUseProgram(m_simple.Program);
    glUniformMatrix4fv(m_simple.Uniforms.Projection, 1, 0, projection.Pointer());
}

void RenderingEngine::Render(float theta) const
{
    // Render the background image:
    {
        const float distance = 10;
        const vec3 target(0, 0, 0);
        const vec3 up(0, 1, 0);
        
        vec3 eye(0, 0, distance * 2);
        mat4 view = mat4::LookAt(eye, target, up);
        
        glUseProgram(m_simple.Program);
        glUniformMatrix4fv(m_simple.Uniforms.Modelview, 1, 0, view.Pointer());
        glDepthFunc(GL_ALWAYS);
        glBindTexture(GL_TEXTURE_2D, m_textures.Tiger);
        
        RenderDrawable(m_quad, m_simple);
    }
        
    const float distance = 10;
    const vec3 target(0, 0, 0);
    const vec3 up(0, 1, 0);
    const vec3 eye(0, 0, distance);
    const mat4 view = mat4::LookAt(eye, target, up);
    const mat4 model = mat4::RotateY(theta * 180.0f / 3.14f);
    const mat4 modelview = model * view;

    vec4 lightWorldSpace = vec4(vec3(0.25, 0.25, 1).Normalized(), 1.0f);
    vec4 lightObjectSpace = model * lightWorldSpace;

    vec4 eyeWorldSpace(0, 0, 1, 1);
    vec4 eyeObjectSpace = model * eyeWorldSpace;

    glUseProgram(m_bump.Program);
    glUniform3fv(m_bump.Uniforms.LightVector,
                1, lightObjectSpace.Pointer());
    glUniform3fv(m_bump.Uniforms.EyeVector, 1, eyeObjectSpace.Pointer());
    glUniformMatrix4fv(m_bump.Uniforms.Modelview, 1,
                       0, modelview.Pointer());
    glBindTexture(GL_TEXTURE_2D, m_textures.TangentSpaceNormals);
    
    // Render the Klein Bottle:
    glEnableVertexAttribArray(m_bump.Attributes.Normal);
    glDepthFunc(GL_LESS);
    RenderDrawable(m_kleinBottle, m_bump);
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
    attribs.TextureCoord = glGetAttribLocation(programHandle, "TextureCoordIn");
    attribs.Tangent = glGetAttribLocation(programHandle, "Tangent");
    
    UniformHandles& uniforms = program.Uniforms;
    uniforms.Projection = glGetUniformLocation(programHandle,  "Projection");
    uniforms.Modelview = glGetUniformLocation(programHandle,  "Modelview");
    uniforms.Sampler = glGetUniformLocation(programHandle,  "Sampler");
    uniforms.LightVector = glGetUniformLocation(programHandle,  "LightVector");
    uniforms.EyeVector = glGetUniformLocation(programHandle,  "EyeVector");
    uniforms.AmbientMaterial = glGetUniformLocation(programHandle,  "AmbientMaterial");
    uniforms.DiffuseMaterial = glGetUniformLocation(programHandle,  "DiffuseMaterial");
    uniforms.SpecularMaterial = glGetUniformLocation(programHandle,  "SpecularMaterial");
    uniforms.Shininess = glGetUniformLocation(programHandle,  "Shininess"); 
}
    
void RenderingEngine::RenderDrawable(const Drawable& drawable,
                                     const ProgramHandles& program) const
{
    int stride = sizeof(vec3);

    if (drawable.Flags & VertexFlagsTexCoords)
        stride += sizeof(vec2);

    if (drawable.Flags & VertexFlagsNormals)
        stride += sizeof(vec3);
    
    if (drawable.Flags & VertexFlagsTangents)
        stride += sizeof(vec3);
    
    int tcOffset = sizeof(vec3);
    int normalOffset = tcOffset + sizeof(vec2);
    int tangentOffset = normalOffset + sizeof(vec3);

    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    
    const AttributeHandles& a = program.Attributes;
    
    glEnableVertexAttribArray(a.Position);
    glVertexAttribPointer(a.Position, 3, GL_FLOAT, GL_FALSE, stride, 0);
    
    if (drawable.Flags & VertexFlagsTexCoords) {
        glEnableVertexAttribArray(a.TextureCoord);
        glVertexAttribPointer(a.TextureCoord, 2, GL_FLOAT, GL_FALSE, stride, (void*) tcOffset);
    }

    if (drawable.Flags & VertexFlagsNormals) {
        glEnableVertexAttribArray(a.Normal);
        glVertexAttribPointer(a.Normal, 3, GL_FLOAT, GL_FALSE, stride, (void*) normalOffset);
    }

    if (drawable.Flags & VertexFlagsTangents) {
        glEnableVertexAttribArray(a.Tangent);
        glVertexAttribPointer(a.Tangent, 3, GL_FLOAT, GL_FALSE, stride, (void*) tangentOffset);
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    
    glDisableVertexAttribArray(a.Position);
    glDisableVertexAttribArray(a.TextureCoord);
    glDisableVertexAttribArray(a.Normal);
    glDisableVertexAttribArray(a.Tangent);
}
    
}