#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "IRenderingEngine.hpp"
#include <vector>

using namespace std;

struct Vertex {
    vec3 Position;
    vec4 Color;
};

class VboRenderingEngine1 : public IRenderingEngine {
public:
    VboRenderingEngine1();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep) {}
    void OnRotate(DeviceOrientation newOrientation) {}
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
private:
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
    GLfloat m_rotationAngle;
    GLfloat m_scale;
    ivec2 m_pivotPoint;
    GLuint m_framebuffer;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    int m_diskIndexCount;
    int m_bodyIndexCount;
};

IRenderingEngine* CreateVboRenderer1()
{
    return new VboRenderingEngine1();
}

VboRenderingEngine1::VboRenderingEngine1() : m_rotationAngle(0), m_scale(1)
{
    // Create & bind the color buffer so that the caller can allocate its space.
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void VboRenderingEngine1::Initialize(int width, int height)
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
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES,
                             GL_DEPTH_COMPONENT16_OES,
                             width,
                             height);
    
    // Create the framebuffer object; attach the depth and color buffers.
    glGenFramebuffersOES(1, &m_framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                 GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES,
                                 m_colorRenderbuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
                                 GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES,
                                 m_depthRenderbuffer);
    
    // Bind the color buffer for rendering.
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glFrustumf(-1.6f, 1.6, -2.4, 2.4, 5, 10);
    
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0, 0, -7);
}

void VboRenderingEngine1::Render() const
{
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glRotatef(m_rotationAngle, 0, 0, 1);
    glScalef(m_scale, m_scale, m_scale);

    const GLvoid* colorOffset = (GLvoid*) sizeof(vec3);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
    glColorPointer(4, GL_FLOAT, sizeof(Vertex), colorOffset);
    glEnableClientState(GL_VERTEX_ARRAY);

    const GLvoid* bodyOffset = 0;
    const GLvoid* diskOffset = (GLvoid*) m_bodyIndexCount;

    glEnableClientState(GL_COLOR_ARRAY);
    glDrawElements(GL_TRIANGLES, m_bodyIndexCount, GL_UNSIGNED_BYTE, bodyOffset);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4f(1, 1, 1, 1);
    glDrawElements(GL_TRIANGLES, m_diskIndexCount, GL_UNSIGNED_BYTE, diskOffset);

    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

void VboRenderingEngine1::OnFingerUp(ivec2 location)
{
    m_scale = 1.0f;
}

void VboRenderingEngine1::OnFingerDown(ivec2 location)
{
    m_scale = 1.5f;
    OnFingerMove(location, location);
}

void VboRenderingEngine1::OnFingerMove(ivec2 previous, ivec2 location)
{
    vec2 direction = vec2(location - m_pivotPoint).Normalized();

    // Flip the Y axis because pixel coords increase towards the bottom.
    direction.y = -direction.y;
    
    m_rotationAngle = std::acos(direction.y) * 180.0f / 3.14159f;
    if (direction.x > 0)
        m_rotationAngle = -m_rotationAngle;
}
