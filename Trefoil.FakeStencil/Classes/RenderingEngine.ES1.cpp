#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

using namespace std;

namespace ES1 {

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};
    
struct Drawables {
    Drawable Knot;
    Drawable Disk;
    Drawable Quad;
};

struct Textures {
    GLuint Grille;
    GLuint Marble;
    GLuint Rhino;
    GLuint Tiger;
};

struct Renderbuffers {
    GLuint Color;
    GLuint Depth;
};

const float DiskY = -1.25f;
const float KnotY = 0.75f;
const float NearPlane = 5;
const float FarPlane = 50;

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(float theta) const;
private:
    GLuint CreateTexture(const string& file, bool npot);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    void UploadImage(const TextureDescription& description);
    Drawables m_drawables;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    IResourceManager* m_resourceManager;
    mat4 m_projection;
    mat4 m_mirror;
};

IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_renderbuffers.Color);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Color);
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
    PrettyPrintExtensions();

    // Create vertex buffer objects.
    m_drawables.Knot = CreateDrawable(TrefoilKnot(2));
    m_drawables.Disk = CreateDrawable(Cone(0.1f, 2.5f));
    m_drawables.Quad = CreateDrawable(Quad(2));
    
    // Load up some textures.
    m_textures.Grille = CreateTexture("Grille.pvr", false);
    m_textures.Marble = CreateTexture("Marble.pvr", false);
    m_textures.Rhino = CreateTexture("Rhino565.pvr", true);
    m_textures.Tiger = CreateTexture("Tiger565.pvr", true);
    
    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);
    glViewport(0, 0, width, height);
    
    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffersOES(1, &m_renderbuffers.Depth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Depth);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT24_OES, width, height);

    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Color);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Depth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Color);
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // Compute the two projection matrices.
    const float aspect = (float) height / width;
    const float shift = -1.25;
    m_projection = mat4::Frustum(-1, 1, -aspect, aspect, NearPlane, FarPlane);
    m_mirror = mat4::Frustum(-1, 1, aspect + shift, -aspect + shift, NearPlane, FarPlane);
    
    // Set up some OpenGL state.
    glEnable(GL_LIGHT0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, vec4(1, 1, 1, 1).Pointer());
    glEnable(GL_TEXTURE_2D);
}

void RenderingEngine::Render(float theta) const
{
    glClear(GL_DEPTH_BUFFER_BIT);

    // Set up the transforms for the background.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-0.5, 0.5, -0.5, 0.5, NearPlane, FarPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -NearPlane * 2);

    // Render the dark background with alpha = 0.
    glDisable(GL_DEPTH_TEST);
    glColor4f(0.5, 0.5, 0.5, 0);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tiger);
    RenderDrawable(m_drawables.Quad);

    // Set up the transforms for the 3D scene.
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projection.Pointer());
    glMatrixMode(GL_MODELVIEW);
    glRotatef(20, 1, 0, 0);
    glBindTexture(GL_TEXTURE_2D, m_textures.Marble);

    // Render the disk normally.
    glColor4f(1, 1, 1, 1);
    glTranslatef(0, DiskY, 0);
    RenderDrawable(m_drawables.Disk);
    glTranslatef(0, -DiskY, 0);
    glEnable(GL_DEPTH_TEST);

    // Render the reflection.
    glPushMatrix();
    glRotatef(theta, 0, 1, 0);
    glTranslatef(0, KnotY, 0);
    glEnable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, m_textures.Grille);
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    glEnable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_mirror.Pointer());
    RenderDrawable(m_drawables.Knot);
    glLoadMatrixf(m_projection.Pointer());
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glPopMatrix();

    // Render the disk again to make the reflection fade out.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, m_textures.Marble);    
    glColor4f(1, 1, 1, 0.5);
    glDisable(GL_DEPTH_TEST);
    glTranslatef(0, DiskY, 0);
    RenderDrawable(m_drawables.Disk);
    glTranslatef(0, -DiskY, 0);
    glEnable(GL_DEPTH_TEST);
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);

    // Clear the depth buffer.
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render the floating object.
    glEnable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, m_textures.Grille);
    glPushMatrix();
    glTranslatef(0, KnotY, 0);
    glRotatef(theta, 0, 1, 0);
    RenderDrawable(m_drawables.Knot);
    glPopMatrix();
    glDisable(GL_LIGHTING);
}

GLuint RenderingEngine::CreateTexture(const string& file, bool npot)
{
    GLuint name;
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    GLenum minFilter = npot ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (npot) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    UploadImage(m_resourceManager->LoadImage(file));
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

void RenderingEngine::UploadImage(const TextureDescription& desc)
{
    GLenum type;
    GLenum format;
    int bitsPerPixel;
    bool compressed = false;
    
    switch (desc.Format) {
        case TextureFormatPvrtcRgba2:
            compressed = true;
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            break;
        case TextureFormatPvrtcRgb2:
            compressed = true;
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            break;
        case TextureFormat565:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            bitsPerPixel = 16;
            break;
        case TextureFormatRgb:
            format = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 24;
            break;
        case TextureFormatRgba:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 32;
            break;
    }
    
    int w = desc.Size.x;
    int h = desc.Size.y;
    unsigned char* data = (unsigned char*) m_resourceManager->GetImageData();
    
    int level = 0;
    while (w && h) {
        GLsizei size = w * h * bitsPerPixel / 8;
        
        if (compressed) {
            size = max(size, 32);
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, size, data);
        }
        else
            glTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, format, type, data);

        if (level < desc.MipCount - 1)
            data += size;
        
        w >>= 1; h >>= 1; level++;
    }

    m_resourceManager->UnloadImage();
}

}