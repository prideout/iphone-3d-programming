#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include <iostream>

namespace ES2 {

#define STRINGIFY(A)  #A
#include "../Shaders/Simple.vert"
#include "../Shaders/Simple.frag"

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLint m_projectionUniform;
    GLint m_modelviewUniform;
    GLuint m_positionSlot;
    GLuint m_colorSlot;
    mat4 m_translation;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine()
{
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
}

void RenderingEngine::Initialize(const vector<ISurface*>& surfaces)
{
    vector<ISurface*>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
        
        // Create the VBO for the vertices.
        vector<float> vertices;
        (*surface)->GenerateVertices(vertices);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(vertices[0]),
                     &vertices[0],
                     GL_STATIC_DRAW);
        
        // Create a new VBO for the indices if needed.
        int indexCount = (*surface)->GetLineIndexCount();
        GLuint indexBuffer;
        if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
            indexBuffer = m_drawables[0].IndexBuffer;
        } else {
            vector<GLushort> indices(indexCount);
            (*surface)->GenerateLineIndices(indices);
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         indexCount * sizeof(GLushort),
                         &indices[0],
                         GL_STATIC_DRAW);
        }
        
        Drawable drawable = { vertexBuffer, indexBuffer, indexCount};
        m_drawables.push_back(drawable);
    }
    
    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    
    // Create the GLSL program.
    GLuint simpleProgram = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(simpleProgram);
    m_positionSlot = glGetAttribLocation(simpleProgram, "Position");
    m_colorSlot = glGetAttribLocation(simpleProgram, "SourceColor");
    glEnableVertexAttribArray(m_positionSlot);

    // Set up some matrices.
    m_translation = mat4::Translate(0, 0, -7);
    m_projectionUniform = glGetUniformLocation(simpleProgram, "Projection");
    m_modelviewUniform = glGetUniformLocation(simpleProgram, "Modelview");
}

void RenderingEngine::Render(const vector<Visual>& visuals) const
{
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
        
    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {

        // Set the viewport transform.
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);

        // Set the model-view transform.
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelview = rotation * m_translation;
        glUniformMatrix4fv(m_modelviewUniform, 1, 0, modelview.Pointer());

        // Set the projection transform.
        float h = 4.0f * size.y / size.x;
        mat4 projectionMatrix = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
        glUniformMatrix4fv(m_projectionUniform, 1, 0, projectionMatrix.Pointer());
        
        // Set the color.
        vec3 color = visual->Color;
        glVertexAttrib4f(m_colorSlot, color.x, color.y, color.z, 1);
        
        // Draw the wireframe.
        int stride = sizeof(vec3);
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexAttribPointer(m_positionSlot, 3, GL_FLOAT, GL_FALSE, stride, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_LINES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
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
