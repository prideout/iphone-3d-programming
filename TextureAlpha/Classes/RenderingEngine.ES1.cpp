#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "ParametricSurface.hpp"
#include <iostream>

using namespace std;

namespace ES1 {

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(float theta) const;
private:
    GLuint m_colorRenderbuffer;
    IResourceManager* m_resourceManager;
    Drawable m_quad;
    GLuint m_straight;
    GLuint m_premultiplied;
};
    
IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

void PrettyPrintExtensions()
{
    char extensions[512];
    strncpy(extensions, (char*) glGetString(GL_EXTENSIONS), sizeof(extensions));
    char* extensionStart = &extensions[0];
    char** extension = &extensionStart;
    cout << "Supported OpenGL ES Extensions:" << endl;
    while (*extension)
        cout << '\t' << strsep(extension, " ") << endl;
    cout << endl;
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    PrettyPrintExtensions();
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    
}

void RenderingEngine::Initialize()
{
    Quad quad(2, 2);
//    Quad quad(3, 3);
    ParametricSurface* surface = &quad;
    
    // Create the VBO for the vertices.
    vector<float> vertices;
    unsigned char vertexFlags = VertexFlagsTexCoords;
    surface->GenerateVertices(vertices, vertexFlags);
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(vertices[0]),
                 &vertices[0],
                 GL_STATIC_DRAW);
    
    // Create a new VBO for the indices if needed.
    int indexCount = surface->GetTriangleIndexCount();
    vector<GLushort> indices(indexCount);
    surface->GenerateTriangleIndices(indices);
    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indexCount * sizeof(GLushort),
                 &indices[0],
                 GL_STATIC_DRAW);

    m_quad.VertexBuffer = vertexBuffer;
    m_quad.IndexBuffer = indexBuffer;
    m_quad.IndexCount = indexCount;

    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);

    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);

    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glOrthof(0, 0, 1, 1, 0, 1);
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    // This loads in a straight texture on the simulator, but
    // loads in a straight texture on the device, due to
    // differences in the Core Graphics implementation.
    glGenTextures(1, &m_straight);
    glBindTexture(GL_TEXTURE_2D, m_straight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    TextureDescription desc = m_resourceManager->LoadPngImage("Circle.png");
    GLvoid* pixels = m_resourceManager->GetImageData();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, desc.Size.x, desc.Size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    m_resourceManager->UnloadImage();

    // Load in the premultiplied texture.
    glGenTextures(1, &m_premultiplied);
    glBindTexture(GL_TEXTURE_2D, m_premultiplied);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    desc = m_resourceManager->LoadImage("Circle.png");
    pixels = m_resourceManager->GetImageData();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, desc.Size.x, desc.Size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    m_resourceManager->UnloadImage();

    // Set up the VBO state.
    int stride = sizeof(vec3) +  sizeof(vec2);
    const GLvoid* texCoordOffset = (const GLvoid*) sizeof(vec3);
    const Drawable& drawable = m_quad;
    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
}

void RenderingEngine::Render(float theta) const
{
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glRotatef(theta, 0, 0, 1);
    
    // Straight Alpha, No Blending
    glBindTexture(GL_TEXTURE_2D, m_straight);
    glDisable(GL_BLEND);
    glViewport(0, 320, 160, 160);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);

    // Straight Alpha, Blend onto Gray
    glEnable(GL_BLEND);
    glViewport(0, 160, 160, 160);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);

    // Straight Alpha, Blend onto White
    glViewport(0, 0, 160, 160);
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);

    // Premultiplied Alpha, No Blending
    glBindTexture(GL_TEXTURE_2D, m_premultiplied);
    glDisable(GL_BLEND);
    glViewport(160, 320, 160, 160);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);

    // Premultiplied Alpha, Blend onto Gray
    glEnable(GL_BLEND);
    glViewport(160, 160, 160, 160);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);

    // Premultiplied Alpha, Blend onto White
    glViewport(160, 0, 160, 160);
    glDisable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);
    glEnable(GL_TEXTURE_2D);
    glDrawElements(GL_TRIANGLES, m_quad.IndexCount, GL_UNSIGNED_SHORT, 0);
    glPopMatrix();
}

}