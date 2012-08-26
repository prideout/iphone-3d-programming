#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include <iostream>

using namespace std;

namespace ES1 {

const float NearPlane = 5;
const float FarPlane = 50;
const float DiskY = -1.25f;
const float ObjectY = 0.75f;

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};
    
struct Drawables {
    Drawable Knot;
    Drawable Bottle;
    Drawable Disk;
    Drawable Quad;
};

struct Textures {
    GLuint Marble;
    GLuint RhinoBackground;
    GLuint TigerBackground;
    GLuint OffscreenSurface;
};

struct Renderbuffers {
    GLuint SmallColor;
    GLuint BigColor;
    GLuint BigDepth;
    GLuint BigStencil;
};

struct Framebuffers {
    GLuint Small;
    GLuint Big;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(float objectTheta, float fboTheta) const;
private:
    ivec2 GetFboSize() const;
    GLuint LoadTexture(const string& file, bool npot);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    void UploadImage(const TextureDescription& description);
    Drawables m_drawables;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    IResourceManager* m_resourceManager;
    mat4 m_projection;
    mat4 m_mirrorProjection;
};

IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_renderbuffers.SmallColor);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SmallColor);
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

ivec2 RenderingEngine::GetFboSize() const
{
    ivec2 size;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &size.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &size.y);
    return size;
}
    
void RenderingEngine::Initialize()
{
    // Create the on-screen FBO.
    
    glGenFramebuffersOES(1, &m_framebuffers.Small);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Small);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.SmallColor);
    
    // Create the double-size off-screen FBO.
    
    ivec2 size = GetFboSize() * 2;

    glGenRenderbuffersOES(1, &m_renderbuffers.BigColor);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.BigColor);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES,
                             size.x, size.y);

    glGenRenderbuffersOES(1, &m_renderbuffers.BigDepth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.BigDepth);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT24_OES,
                             size.x, size.y);

    glGenRenderbuffersOES(1, &m_renderbuffers.BigStencil);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.BigStencil);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_STENCIL_INDEX8_OES,
                             size.x, size.y);

    glGenFramebuffersOES(1, &m_framebuffers.Big);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Big);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.BigColor);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                    GL_RENDERBUFFER_OES, m_renderbuffers.BigDepth);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.BigStencil);

    // Create a texture object and associate it with the big FBO.
    
    glGenTextures(1, &m_textures.OffscreenSurface);
    glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenSurface);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                              GL_TEXTURE_2D, m_textures.OffscreenSurface, 0);

    // Check FBO status.
    
    GLenum status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        cout << "Incomplete FBO" << endl;
        exit(1);
    }
    
    // Load up some textures.
    m_textures.Marble = LoadTexture("Marble.pvr", false);
    m_textures.RhinoBackground = LoadTexture("Rhino565.pvr", true);
    m_textures.TigerBackground = LoadTexture("Tiger565.pvr", true);

    // Create vertex buffer objects.
    m_drawables.Knot = CreateDrawable(TrefoilKnot(2));
    m_drawables.Bottle = CreateDrawable(KleinBottle(0.2));
    m_drawables.Disk = CreateDrawable(Cone(0.1f, 2.5f));
    m_drawables.Quad = CreateDrawable(Quad(2));

    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // Compute the two projection matrices.
    const float aspect = (float) size.y / size.x;
    const float shift = -1.25;
    float bottom = -aspect, top = aspect;
    m_projection = mat4::Frustum(-1, 1, bottom, top, NearPlane, FarPlane);
    m_mirrorProjection = mat4::Frustum(-1, 1, top + shift, bottom + shift, NearPlane, FarPlane);
}
    
void RenderingEngine::Render(float objectTheta, float fboTheta) const
{
    Drawable drawable;
    GLuint background;
    vec3 color;
    
    // Look at fboTheta to determine which "side" should be rendered:
    //   1) Orange Trefoil Knot against a Tiger background
    //   2) Green Klein bottle against a Rhino background
    
    if (fboTheta > 270 || fboTheta < 90) {
        background = m_textures.TigerBackground;
        drawable = m_drawables.Knot;
        color = vec3(1, 0.5, 0.1);
    } else {
        background = m_textures.RhinoBackground;
        drawable = m_drawables.Bottle;
        objectTheta = -objectTheta;
        color = vec3(0.5, 0.75, 0.1);
    }
    
    // Bind the double-size FBO.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Big);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.BigColor);
    ivec2 bigSize = GetFboSize();
    glViewport(0, 0, bigSize.x, bigSize.y);

    // Draw the 3D scene.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projection.Pointer());
    glMatrixMode(GL_MODELVIEW);
    
    // Clear the back buffer and orient the scene.
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, -NearPlane * 2);
    glRotatef(20, 1, 0, 0);
    
    // Prepare the render state for the disk.
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 0xff, 0xff);

    // Render the disk to the stencil buffer only.
    glTranslatef(0, DiskY, 0);
        glDepthMask(GL_FALSE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                RenderDrawable(m_drawables.Disk);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
    glTranslatef(0, -DiskY, 0);

    // Prepare the render state for the reflection.
    glRotatef(objectTheta, 0, 1, 0);
    glTranslatef(0, ObjectY, 0);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 0xff, 0xff);
    glEnable(GL_LIGHTING);

    float a = 0.4;
    vec4 diffuse = vec4(color * a, 1 - a);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.Pointer());
    
    // Render the reflection.
    glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(m_mirrorProjection.Pointer());
            RenderDrawable(drawable);
        glLoadMatrixf(m_projection.Pointer());
    glMatrixMode(GL_MODELVIEW);

    // Turn off the stencil test and clear the depth buffer.
    glDisable(GL_STENCIL_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    diffuse = vec4(color, 0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.Pointer());

    // Render the floating object.
    RenderDrawable(drawable);

    // Prepare for rendering the disk.
    glTranslatef(0, DiskY - ObjectY, 0);
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, m_textures.Marble);
    glBlendFuncSeparateOES(GL_DST_ALPHA, GL_ONE,             // RGB factors
                           GL_ZERO, GL_ONE_MINUS_SRC_ALPHA); // Alpha factors
    glEnable(GL_TEXTURE_2D);
    glRotatef(-objectTheta, 0, 1, 0);

    // Render the disk.
    glEnable(GL_BLEND);
    RenderDrawable(m_drawables.Disk);

    // Render the background.
    glColor4f(0.7, 0.7, 0.7, 1);
    glBindTexture(GL_TEXTURE_2D, background);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-0.5, 0.5, -0.5, 0.5, NearPlane, FarPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -NearPlane * 2);
    RenderDrawable(m_drawables.Quad);
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    
    // Switch to the on-screen render target.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Small);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SmallColor);
    ivec2 smallSize = GetFboSize();
    glViewport(0, 0, smallSize.x, smallSize.y);

    // Clear the color buffer only if necessary.
    if ((int) fboTheta % 180) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Render the offscreen surface by applying it to a quad.
    glDisable(GL_DEPTH_TEST);
    glRotatef(fboTheta, 0, 1, 0);
    glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenSurface);
    RenderDrawable(m_drawables.Quad);
    glDisable(GL_TEXTURE_2D);
}

GLuint RenderingEngine::LoadTexture(const string& file, bool npot)
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