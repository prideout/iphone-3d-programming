#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "RenderingBase.hpp"
#include <iostream>

// This file has a certain warning turned off: "-Wno-invalid-offsetof"
// It's safe to use offsetof on non-POD types, as long as they're fairly simple.
#define _offsetof(TYPE, MEMBER) (GLvoid*) (offsetof(TYPE, MEMBER))

void ReadShaderFile(const std::string& file, std::string& contents);

using namespace std;

namespace ES2 {

struct UniformHandles {
    GLint Projection;
    GLint Modelview;
};

struct AttributeHandles {
    GLint Position;
    GLint TexCoord;
    GLint BoneWeights;
    GLint BoneIndices;
};

struct ProgramHandles {
    GLuint Program;
    AttributeHandles Attributes;
    UniformHandles Uniforms;
};
    

class RenderingEngine : public RenderingBase, public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render() const;
    void UpdateAnimation(float timestamp);
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    void BuildProgram(const char* vs, const char* fs, ProgramHandles&) const;
    ProgramHandles m_blitting;
    ProgramHandles m_skinning;
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

void RenderingEngine::Initialize()
{
    IndexList indices;
    VertexList vertices;
    RenderingBase::Initialize(BoneCount, vertices, indices);
    
    glGenBuffers(1, &m_skinnedFigure.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skinnedFigure.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(indices[0]),
                 &indices[0],
                 GL_STATIC_DRAW);

    glGenBuffers(1, &m_skinnedFigure.VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_skinnedFigure.VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(vertices[0]),
                 &vertices[0],
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Extract width and height from the color buffer:
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_WIDTH, &m_screenSize.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_HEIGHT, &m_screenSize.y);
    
    // Create the on-screen FBO:
    glGenFramebuffers(1, &m_framebuffers.Screen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Screen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.Screen);
    
    // Set up various GL state:
    glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, m_screenSize.x, m_screenSize.y);

    // Create the GLSL programs:
    BuildProgram("Blitting.es2.vert", "Blitting.es2.frag", m_blitting);
    BuildProgram("Skinning.es2.vert", "Skinning.es2.frag", m_skinning);
}
    
void RenderingEngine::UpdateAnimation(float timestamp)
{
    AnimateSkeleton(timestamp, m_skeleton);
    ComputeMatrices(m_skeleton, m_skinnedFigure.Matrices);
}

void RenderingEngine::Render() const
{
    GLsizei stride = sizeof(Vertex);
    mat4 projection = mat4::Ortho(-1, 1, -1.5, 1.5, -100, 100);

    // Draw background:
    glUseProgram(m_blitting.Program);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tile);
    glEnableVertexAttribArray(m_blitting.Attributes.Position);
    glEnableVertexAttribArray(m_blitting.Attributes.TexCoord);
    glVertexAttribPointer(m_blitting.Attributes.Position, 3, GL_FLOAT, GL_FALSE, stride, m_backgroundVertices[0].Position.Pointer());
    glVertexAttribPointer(m_blitting.Attributes.TexCoord, 2, GL_FLOAT, GL_FALSE, stride, m_backgroundVertices[0].TexCoord.Pointer());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(m_blitting.Attributes.Position);
    glDisableVertexAttribArray(m_blitting.Attributes.TexCoord);

    // Render the stick figure:
    glUseProgram(m_skinning.Program);
    glUniformMatrix4fv(m_skinning.Uniforms.Projection, 1, GL_FALSE, projection.Pointer());
    glUniformMatrix4fv(m_skinning.Uniforms.Modelview,
                       m_skinnedFigure.Matrices.size(),
                       GL_FALSE,
                       m_skinnedFigure.Matrices[0].Pointer());
    glBindTexture(GL_TEXTURE_2D, m_textures.Circle);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnableVertexAttribArray(m_skinning.Attributes.Position);
    glEnableVertexAttribArray(m_skinning.Attributes.TexCoord);
    glEnableVertexAttribArray(m_skinning.Attributes.BoneWeights);
    glEnableVertexAttribArray(m_skinning.Attributes.BoneIndices);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skinnedFigure.IndexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_skinnedFigure.VertexBuffer);
    
    glVertexAttribPointer(m_skinning.Attributes.BoneWeights, 2, GL_FLOAT, GL_FALSE, stride, _offsetof(Vertex, BoneWeights));
    glVertexAttribPointer(m_skinning.Attributes.BoneIndices, 2, GL_UNSIGNED_BYTE, GL_FALSE, stride, _offsetof(Vertex, BoneIndices));
    glVertexAttribPointer(m_skinning.Attributes.Position, 3, GL_FLOAT, GL_FALSE, stride, _offsetof(Vertex, Position));
    glVertexAttribPointer(m_skinning.Attributes.TexCoord, 2, GL_FLOAT, GL_FALSE, stride, _offsetof(Vertex, TexCoord));
    
    size_t indicesPerBone = 12 + 6 * (NumDivisions + 1);
    int indexCount = BoneCount * indicesPerBone;
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(m_skinning.Attributes.Position);
    glDisableVertexAttribArray(m_skinning.Attributes.TexCoord);
    glDisableVertexAttribArray(m_skinning.Attributes.BoneWeights);
    glDisableVertexAttribArray(m_skinning.Attributes.BoneIndices);
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
    attribs.TexCoord = glGetAttribLocation(programHandle, "TextureCoordIn");
    attribs.BoneWeights = glGetAttribLocation(programHandle, "BoneWeights");
    attribs.BoneIndices = glGetAttribLocation(programHandle, "BoneIndices");
    
    UniformHandles& uniforms = program.Uniforms;
    uniforms.Projection = glGetUniformLocation(programHandle, "Projection");
    uniforms.Modelview = glGetUniformLocation(programHandle, "Modelview");
}
    
}
