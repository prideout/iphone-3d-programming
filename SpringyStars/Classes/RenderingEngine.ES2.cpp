#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include "../Textures/Star.h"
#include "../Textures/Background.h"
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/SimpleTexturing.es2.vert"
#include "../Shaders/SimpleTexturing.es2.frag"

using namespace std;

namespace ES2 {

#include "RenderingEngine.Common.hpp"

struct UniformHandles {
    GLuint Modelview;
    GLuint Projection;
    GLuint Sampler;
    GLuint IsSprite;
};

struct AttributeHandles {
    GLint Position;
    GLint TextureCoord;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(const PositionList& positions) const;
private:
    void RenderBackground() const;
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    AttributeHandles m_attributes;
    UniformHandles m_uniforms;
    ivec2 m_screenSize;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}
    
RenderingEngine::RenderingEngine()
{
    glGenRenderbuffers(1, &m_renderbuffers.Screen);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Screen);
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
    // Load up some textures:
    m_textures.Star = CreateTexture(Star);
    m_textures.Background = CreateTexture(_Background_pvrtc);
    
    // Extract width and height from the color buffer.
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_WIDTH, &m_screenSize.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_HEIGHT, &m_screenSize.y);
    
    // Create the on-screen FBO.
    glGenFramebuffers(1, &m_framebuffers.Screen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Screen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.Screen);
    
    // Create the GLSL program.
    GLuint program = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(program);
    
    // Extract the handles to attributes and uniforms.
    m_attributes.Position = glGetAttribLocation(program, "Position");
    m_attributes.TextureCoord = glGetAttribLocation(program, "TextureCoord");
    m_uniforms.Projection = glGetUniformLocation(program, "Projection");
    m_uniforms.Modelview = glGetUniformLocation(program, "Modelview");
    m_uniforms.Sampler = glGetUniformLocation(program, "Sampler");
    m_uniforms.IsSprite = glGetUniformLocation(program, "IsSprite");
    
    // Set up various GL state.
    glViewport(0, 0, m_screenSize.x, m_screenSize.y);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // Set up the transforms.
    const float NearPlane = 5, FarPlane = 100;
    const float Scale = 0.0005;
    const float HalfWidth = Scale * m_screenSize.x / 2;
    const float HalfHeight = Scale * m_screenSize.y / 2;
    
    mat4 projection = mat4::Frustum(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight,
                                    NearPlane, FarPlane);
    glUniformMatrix4fv(m_uniforms.Projection, 1, 0, projection.Pointer());

    vec3 eye(0, 0, 40);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);
    mat4 modelview = mat4::LookAt(eye, target, up);
    glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
}

void RenderingEngine::Render(const PositionList& positions) const
{
    RenderBackground();
    
    glBindTexture(GL_TEXTURE_2D, m_textures.Star);
    glEnableVertexAttribArray(m_attributes.Position);
    glDisableVertexAttribArray(m_attributes.TextureCoord);
    glUniform1i(m_uniforms.IsSprite, GL_TRUE);
    
    glVertexAttribPointer(m_attributes.Position,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(vec2),
                          &positions[0].x);
    
    glEnable(GL_BLEND);
    glDrawArrays(GL_POINTS, 0, positions.size());
    glDisable(GL_BLEND);
}

void RenderingEngine::RenderBackground() const
{
    glBindTexture(GL_TEXTURE_2D, m_textures.Background);
    glEnableVertexAttribArray(m_attributes.Position);
    glEnableVertexAttribArray(m_attributes.TextureCoord);
    glUniform1i(m_uniforms.IsSprite, GL_FALSE); // This exposes a bug on the 3GS (looks ok on simulator)
    
    float positions[] = {
        -1, -1.5,
        -1, 1.5,
        1, 1.5,
        
        1, 1.5,
        1, -1.5,
        -1, -1.5,
    };
    
    glVertexAttribPointer(m_attributes.Position,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(vec2),
                          positions);
    
    float texCoords[] = {
        0, 0,
        0, 320.0/512.0,
        480.0/512.0, 320.0/512.0,
        
        480.0/512.0, 320.0/512.0,
        480.0/512.0, 0,
        0, 0,
    };
    
    glVertexAttribPointer(m_attributes.TextureCoord,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(vec2),
                          texCoords);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
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

GLuint RenderingEngine::BuildProgram(const char* vertexShaderSource,
                                     const char* fragmentShaderSource) const
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
    
    return programHandle;
}

}