#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "IRenderingEngine.hpp"
#include "Quaternion.hpp"
#include <vector>
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/Simple.vert"
#include "../Shaders/Simple.frag"

static const float AnimationDuration = 0.25f;

using namespace std;

struct Vertex {
    vec3 Position;
    vec4 Color;
};

struct Animation {
    Quaternion Start;
    Quaternion End;
    Quaternion Current;
    float Elapsed;
    float Duration;
};

class RenderingEngine2 : public IRenderingEngine {
public:
    RenderingEngine2();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep);
    void OnRotate(DeviceOrientation newOrientation);
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    vector<Vertex> m_cone;
    vector<Vertex> m_disk;
    Animation m_animation;
    GLuint m_simpleProgram;
    GLuint m_framebuffer;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
};

IRenderingEngine* CreateRenderer2()
{
    return new RenderingEngine2();
}

RenderingEngine2::RenderingEngine2()
{
    // Create & bind the color buffer so that the caller can allocate its space.
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
}

void RenderingEngine2::Initialize(int width, int height)
{
    const float coneRadius = 0.5f;
    const float coneHeight = 1.866f;
    const int coneSlices = 40;
    
    {
        // Allocate space for the cone vertices.
        m_cone.resize((coneSlices + 1) * 2);
        
        // Initialize the vertices of the triangle strip.
        vector<Vertex>::iterator vertex = m_cone.begin();
        const float dtheta = TwoPi / coneSlices;
        for (float theta = 0; vertex != m_cone.end(); theta += dtheta) {
            
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
    }
    
    {
        // Allocate space for the disk vertices.
        m_disk.resize(coneSlices + 2);
        
        // Initialize the center vertex of the triangle fan.
        vector<Vertex>::iterator vertex = m_disk.begin();
        vertex->Color = vec4(0.75, 0.75, 0.75, 1);
        vertex->Position.x = 0;
        vertex->Position.y = 1 - coneHeight;
        vertex->Position.z = 0;
        vertex++;
        
        // Initialize the rim vertices of the triangle fan.
        const float dtheta = TwoPi / coneSlices;
        for (float theta = 0; vertex != m_disk.end(); theta += dtheta) {
            vertex->Color = vec4(0.75, 0.75, 0.75, 1);
            vertex->Position.x = coneRadius * cos(theta);
            vertex->Position.y = 1 - coneHeight;
            vertex->Position.z = coneRadius * sin(theta);
            vertex++;
        }
    }
    
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
    
    // Set up some GL state.
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    // Build the GLSL program.
    m_simpleProgram = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(m_simpleProgram);

    // Set the projection matrix.
    GLint projectionUniform = glGetUniformLocation(m_simpleProgram, "Projection");
    mat4 projectionMatrix = mat4::Frustum(-1.6f, 1.6, -2.4, 2.4, 5, 10);
    glUniformMatrix4fv(projectionUniform, 1, 0, projectionMatrix.Pointer());
}

void RenderingEngine2::Render() const
{
    GLuint positionSlot = glGetAttribLocation(m_simpleProgram, "Position");
    GLuint colorSlot = glGetAttribLocation(m_simpleProgram, "SourceColor");

    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnableVertexAttribArray(positionSlot);
    glEnableVertexAttribArray(colorSlot);
    
    mat4 rotation(m_animation.Current.ToMatrix());
    mat4 translation = mat4::Translate(0, 0, -7);

    // Set the model-view matrix.
    GLint modelviewUniform = glGetUniformLocation(m_simpleProgram, "Modelview");
    mat4 modelviewMatrix = rotation * translation;
    glUniformMatrix4fv(modelviewUniform, 1, 0, modelviewMatrix.Pointer());
    
    // Draw the cone.
    {
      GLsizei stride = sizeof(Vertex);
      const GLvoid* pCoords = &m_cone[0].Position.x;
      const GLvoid* pColors = &m_cone[0].Color.x;
      glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE, stride, pCoords);
      glVertexAttribPointer(colorSlot, 4, GL_FLOAT, GL_FALSE, stride, pColors);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, m_cone.size());
    }
    
    // Draw the disk that caps off the base of the cone.
    {
      GLsizei stride = sizeof(Vertex);
      const GLvoid* pCoords = &m_disk[0].Position.x;
      const GLvoid* pColors = &m_disk[0].Color.x;
      glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE, stride, pCoords);
      glVertexAttribPointer(colorSlot, 4, GL_FLOAT, GL_FALSE, stride, pColors);
      glDrawArrays(GL_TRIANGLE_FAN, 0, m_disk.size());
    }
    
    glDisableVertexAttribArray(positionSlot);
    glDisableVertexAttribArray(colorSlot);
}

void RenderingEngine2::UpdateAnimation(float timeStep)
{
    if (m_animation.Current == m_animation.End)
        return;
    
    m_animation.Elapsed += timeStep;
    if (m_animation.Elapsed >= AnimationDuration) {
        m_animation.Current = m_animation.End;
    } else {
        float mu = m_animation.Elapsed / AnimationDuration;
        m_animation.Current = m_animation.Start.Slerp(mu, m_animation.End);
    }
}

void RenderingEngine2::OnRotate(DeviceOrientation orientation)
{
    vec3 direction;
    
    switch (orientation) {
        case DeviceOrientationUnknown:
        case DeviceOrientationPortrait:
            direction = vec3(0, 1, 0);
            break;
            
        case DeviceOrientationPortraitUpsideDown:
            direction = vec3(0, -1, 0);
            break;
            
        case DeviceOrientationFaceDown:       
            direction = vec3(0, 0, -1);
            break;
            
        case DeviceOrientationFaceUp:
            direction = vec3(0, 0, 1);
            break;
            
        case DeviceOrientationLandscapeLeft:
            direction = vec3(+1, 0, 0);
            break;
            
        case DeviceOrientationLandscapeRight:
            direction = vec3(-1, 0, 0);
            break;
    }
    
    m_animation.Elapsed = 0;
    m_animation.Start = m_animation.Current = m_animation.End;
    m_animation.End = Quaternion::CreateFromVectors(vec3(0, 1, 0), direction);
}

GLuint RenderingEngine2::BuildShader(const char* source, GLenum shaderType) const
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

GLuint RenderingEngine2::BuildProgram(const char* vertexShaderSource,
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
