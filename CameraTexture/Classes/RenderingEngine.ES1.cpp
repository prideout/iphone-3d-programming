#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include "ParametricSurface.hpp"

#include <iostream>
using namespace std;

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(float zScale, float theta, bool waiting) const;
    void LoadCameraTexture(const TextureDescription& description, void* data);
private:
    GLuint CreateTexture(const string& file);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    void UploadImage(const TextureDescription& description, void* data = 0);
    Drawable m_sphere;
    Drawable m_button;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    GLuint m_cameraTexture;
    GLuint m_waitTexture;
    GLuint m_actionTexture;
    IResourceManager* m_resourceManager;
};
    
IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void RenderingEngine::Initialize()
{
    // Create vertex buffer objects.
    m_sphere = CreateDrawable(Sphere(2.5));
    m_button = CreateDrawable(Quad(4, 1));
    
    // Load up some textures.
    m_cameraTexture = CreateTexture("Tarsier.png");
    m_waitTexture = CreateTexture("PleaseWait.png");
    m_actionTexture = CreateTexture("TakePicture.png");

    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);
    glViewport(0, 0, width, height);

    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
        
    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    // Set up the material properties.
    vec4 diffuse(1, 1, 1, 1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.Pointer());
    
    // Set the light position.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    vec4 lightPosition(0.25, 0.25, 1, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition.Pointer());
    
    // Set the model-view transform.
    mat4 modelview = mat4::Translate(0, 0, -8);
    glLoadMatrixf(modelview.Pointer());
    
    // Set the projection transform.
    float h = 4.0f * height / width;
    mat4 projection = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.Pointer());
    glMatrixMode(GL_MODELVIEW);
}

void RenderingEngine::Render(float zScale, float theta, bool waiting) const
{
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    
    // Draw the button.
    glTranslatef(0, -4, 0);
    glBindTexture(GL_TEXTURE_2D, waiting ? m_waitTexture : m_actionTexture);
    RenderDrawable(m_button);
    
    // Draw the sphere.
    glBindTexture(GL_TEXTURE_2D, m_cameraTexture);
    glTranslatef(0, 4.75, 0);
    glRotatef(theta, 1, 0, 0);
    glScalef(1, 1, zScale);
    glEnable(GL_LIGHTING);
    RenderDrawable(m_sphere);
    glDisable(GL_LIGHTING);

    glPopMatrix();
}

void RenderingEngine::LoadCameraTexture(const TextureDescription& desc, void* data)
{
    glBindTexture(GL_TEXTURE_2D, m_cameraTexture);
    UploadImage(desc, data);
}

GLuint RenderingEngine::CreateTexture(const string& file)
{
    GLuint name;
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    UploadImage(m_resourceManager->LoadImagePot(file));
    return name;
}

Drawable RenderingEngine::CreateDrawable(const ParametricSurface& surface)
{
    // Create the VBO for the vertices.
    vector<float> vertices;
    unsigned char vertexFlags = VertexFlagsNormals | VertexFlagsTexCoords;
    surface.GenerateVertices(vertices, vertexFlags);
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
    vector<GLushort> indices(indexCount);
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
    return drawable;
}

void RenderingEngine::RenderDrawable(const Drawable& drawable) const
{
    int stride = sizeof(vec3) + sizeof(vec3) + sizeof(vec2);
    const GLvoid* normalOffset = (const GLvoid*) sizeof(vec3);
    const GLvoid* texCoordOffset = (const GLvoid*) (2 * sizeof(vec3));
    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glNormalPointer(GL_FLOAT, stride, normalOffset);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
}

void RenderingEngine::UploadImage(const TextureDescription& description, void* data)
{
    GLenum format;
    switch (description.Format) {
        case TextureFormatRgb:  format = GL_RGB;  break;
        case TextureFormatRgba: format = GL_RGBA; break;
    }
    
    GLenum type = GL_UNSIGNED_BYTE;
    ivec2 size = description.Size;
    
    if (data == 0) {
        data = m_resourceManager->GetImageData();
        glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, type, data);
        m_resourceManager->UnloadImage();
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, type, data);
    }
}