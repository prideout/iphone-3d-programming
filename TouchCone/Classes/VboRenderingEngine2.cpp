#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "IRenderingEngine.hpp"
#include "Matrix.hpp"
#include <vector>
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/Simple.vert"
#include "../Shaders/Simple.frag"

using namespace std;

struct Vertex {
    vec3 Position;
    vec4 Color;
};

class VboRenderingEngine2 : public IRenderingEngine {
public:
    VboRenderingEngine2();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep) {}
    void OnRotate(DeviceOrientation newOrientation) {}
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
    GLfloat m_rotationAngle;
    GLfloat m_scale;
    ivec2 m_pivotPoint;
    GLuint m_simpleProgram;
    GLuint m_framebuffer;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    int m_diskIndexCount;
    int m_bodyIndexCount;
};

IRenderingEngine* CreateVboRenderer2()
{
    return new VboRenderingEngine2();
}

VboRenderingEngine2::VboRenderingEngine2() : m_rotationAngle(0), m_scale(1)
{
    // Create & bind the color buffer so that the caller can allocate its space.
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
}

void VboRenderingEngine2::Initialize(int width, int height)
{
    m_pivotPoint = ivec2(width / 2, height / 2);
    
    const float coneRadius = 0.5f;
    const float coneHeight = 1.866f;
    const int coneSlices = 40;
    const float dtheta = TwoPi / coneSlices;
    const int vertexCount = coneSlices * 2 + 1;
    const int diskCenterIndex = vertexCount - 1;

    m_bodyIndexCount = coneSlices * 3;
    m_diskIndexCount = coneSlices * 3;

    vector<Vertex> coneVertices(vertexCount);
    vector<Vertex>::iterator vertex = coneVertices.begin();
    
    // Cone's body
    for (float theta = 0; vertex != coneVertices.end() - 1; theta += dtheta) {
        
        // Grayscale gradient
        float brightness = abs(sin(theta));
        vec4 color(brightness, brightness, brightness, 1);
        
        // Apex vertex
        vertex->Position = vec3(0, 1, 0);
        vertex->Color = color;
        vertex++;
        
        // Rim vertex
        vertex->Position.x = coneRadius * cos(theta);
        vertex->Position.y = 1 - coneHeight;
        vertex->Position.z = coneRadius * sin(theta);
        vertex->Color = color;
        vertex++;
    }
    
    // Disk center
    vertex->Position = vec3(0, 1 - coneHeight, 0);
    vertex->Color = vec4(1, 1, 1, 1);
    
    // Create the VBO for the vertices.
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 coneVertices.size() * sizeof(coneVertices[0]),
                 &coneVertices[0],
                 GL_STATIC_DRAW);
    
    vector<GLubyte> coneIndices(m_bodyIndexCount + m_diskIndexCount);
    vector<GLubyte>::iterator index = coneIndices.begin();
    
    // Body triangles
    for (int i = 0; i < coneSlices * 2; i += 2) {
        *index++ = i;
        *index++ = (i + 1) % (2 * coneSlices);
        *index++ = (i + 3) % (2 * coneSlices);
    }
    
    // Disk triangles
    for (int i = 1; i < coneSlices * 2 + 1; i += 2) {
        *index++ = diskCenterIndex;
        *index++ = i;
        *index++ = (i + 2) % (2 * coneSlices);
    }
    
    // Create the VBO for the indices.
    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 coneIndices.size() * sizeof(coneIndices[0]),
                 &coneIndices[0],
                 GL_STATIC_DRAW);
        
    // Create the depth buffer.
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER,
                             GL_DEPTH_COMPONENT16,
                             width,
                             height);
    
    // Create the framebuffer object; attach the depth and color buffers.
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                 GL_COLOR_ATTACHMENT0,
                                 GL_RENDERBUFFER,
                                 m_colorRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                 GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER,
                                 m_depthRenderbuffer);
    
    // Bind the color buffer for rendering.
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    
    m_simpleProgram = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(m_simpleProgram);
    
    // Set the projection matrix.
    GLint projectionUniform = glGetUniformLocation(m_simpleProgram, "Projection");
    mat4 projectionMatrix = mat4::Frustum(-1.6f, 1.6, -2.4, 2.4, 5, 10);
    glUniformMatrix4fv(projectionUniform, 1, 0, projectionMatrix.Pointer());
}

void VboRenderingEngine2::Render() const
{
    GLuint positionSlot = glGetAttribLocation(m_simpleProgram, "Position");
    GLuint colorSlot = glGetAttribLocation(m_simpleProgram, "SourceColor");

    mat4 rotation = mat4::Rotate(m_rotationAngle);
    mat4 scale = mat4::Scale(m_scale);
    mat4 translation = mat4::Translate(0, 0, -7);
    GLint modelviewUniform = glGetUniformLocation(m_simpleProgram, "Modelview");
    mat4 modelviewMatrix = scale * rotation * translation;

    GLsizei stride = sizeof(Vertex);
    const GLvoid* colorOffset = (GLvoid*) sizeof(vec3);

    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(modelviewUniform, 1, 0, modelviewMatrix.Pointer());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glVertexAttribPointer(colorSlot, 4, GL_FLOAT, GL_FALSE, stride, colorOffset);
    glEnableVertexAttribArray(positionSlot);
    
    const GLvoid* bodyOffset = 0;
    const GLvoid* diskOffset = (GLvoid*) m_bodyIndexCount;
    
    glEnableVertexAttribArray(colorSlot);
    glDrawElements(GL_TRIANGLES, m_bodyIndexCount, GL_UNSIGNED_BYTE, bodyOffset);
    glDisableVertexAttribArray(colorSlot);
    glVertexAttrib4f(colorSlot, 1, 1, 1, 1);
    glDrawElements(GL_TRIANGLES, m_diskIndexCount, GL_UNSIGNED_BYTE, diskOffset);
    
    glDisableVertexAttribArray(positionSlot);
}

void VboRenderingEngine2::OnFingerUp(ivec2 location)
{
    m_scale = 1.0f;
}

void VboRenderingEngine2::OnFingerDown(ivec2 location)
{
    m_scale = 1.5f;
    OnFingerMove(location, location);
}

void VboRenderingEngine2::OnFingerMove(ivec2 previous, ivec2 location)
{
    vec2 direction = vec2(location - m_pivotPoint).Normalized();
    
    // Flip the Y axis because pixel coords increase towards the bottom.
    direction.y = -direction.y;
    
    m_rotationAngle = std::acos(direction.y) * 180.0f / 3.14159f;
    if (direction.x > 0)
        m_rotationAngle = -m_rotationAngle;
}

GLuint VboRenderingEngine2::BuildShader(const char* source, GLenum shaderType) const
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

GLuint VboRenderingEngine2::BuildProgram(const char* vertexShaderSource,
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
