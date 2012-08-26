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
    Drawable Bottle;
    Drawable Disk;
    Drawable Quad;
};

struct Framebuffers {
    GLuint Accumulated;
    GLuint Scene;
};

struct Renderbuffers {
    GLuint AccumulatedColor;
    GLuint SceneColor;
    GLuint SceneDepth;
    GLuint SceneStencil;
};

struct Textures {
    GLuint Marble;
    GLuint RhinoBackground;
    GLuint TigerBackground;
    GLuint OffscreenSurface;
};
    
class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(float objectTheta, float fboTheta) const;
private:
    void RenderPass(float objectTheta, float fboTheta, vec2 jitterOffset) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    GLuint CreateTexture(const string& file, bool npot);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    void UploadImage(const TextureDescription& description);
    Drawables m_drawables;
    vec2 m_viewport;
    IResourceManager* m_resourceManager;
};

IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_renderbuffers.AccumulatedColor);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.AccumulatedColor);
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
    m_drawables.Bottle = CreateDrawable(KleinBottle(0.2));
    m_drawables.Disk = CreateDrawable(Cone(0.1f, 2.5f));
    m_drawables.Quad = CreateDrawable(Quad(2));
    
    // Load up some textures.
    m_textures.Marble = CreateTexture("Marble.pvr", false);
    m_textures.RhinoBackground = CreateTexture("Rhino565.pvr", true);
    m_textures.TigerBackground = CreateTexture("Tiger565.pvr", true);
    
    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);
    m_viewport = vec2(width, height);
    
    // Create the on-screen FBO.
    glGenFramebuffersOES(1, &m_framebuffers.Accumulated);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Accumulated);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.AccumulatedColor);
    
    // Create the off-screen FBO.
    
    glGenRenderbuffersOES(1, &m_renderbuffers.SceneColor);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SceneColor);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES, width, height);

    glGenRenderbuffersOES(1, &m_renderbuffers.SceneDepth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SceneDepth);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT24_OES, width, height);

    glGenRenderbuffersOES(1, &m_renderbuffers.SceneStencil);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SceneStencil);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_STENCIL_INDEX8_OES, width, height);

    glGenFramebuffersOES(1, &m_framebuffers.Scene);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Scene);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.SceneColor);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                    GL_RENDERBUFFER_OES, m_renderbuffers.SceneDepth);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.SceneStencil);

    // Create a texture object and associate it with the big FBO.
    glGenTextures(1, &m_textures.OffscreenSurface);
    glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenSurface);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                              GL_TEXTURE_2D, m_textures.OffscreenSurface, 0);

    // check FBO status
    GLenum status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        cout << "Incomplete FBO" << endl;
        exit(1);
    }
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glViewport(0, 0, m_viewport.x, m_viewport.y);
}

void RenderingEngine::Render(float objectTheta, float fboTheta) const
{
    const vec2 JitterOffsets2[2] =
    {
        vec2(0.25f, 0.75f), vec2(0.75f, 0.25f),
    };

    const vec2 JitterOffsets4[4] =
    {
        vec2(0.375f, 0.25f), vec2(0.125f, 0.75f),
        vec2(0.875f, 0.25f), vec2(0.625f, 0.75f),
    };

    const vec2 JitterOffsets8[8] =
    {
        vec2(0.5625f, 0.4375f), vec2(0.0625f, 0.9375f),
        vec2(0.3125f, 0.6875f), vec2(0.6875f, 0.8125f),
        
        vec2(0.8125f, 0.1875f), vec2(0.9375f, 0.5625f),
        vec2(0.4375f, 0.0625f), vec2(0.1875f, 0.3125f),
    };

    const vec2 JitterOffsets16[16] =
    {
        vec2(0.375f, 0.4375f), vec2(0.625f, 0.0625f),
        vec2(0.875f, 0.1875f), vec2(0.125f, 0.0625f),
        
        vec2(0.375f, 0.6875f), vec2(0.875f, 0.4375f),
        vec2(0.625f, 0.5625f), vec2(0.375f, 0.9375f),
        
        vec2(0.625f, 0.3125f), vec2(0.125f, 0.5625f),
        vec2(0.125f, 0.8125f), vec2(0.375f, 0.1875f),
        
        vec2(0.875f, 0.9375f), vec2(0.875f, 0.6875f),
        vec2(0.125f, 0.3125f), vec2(0.625f, 0.8125f),
    };

    const int JitterCount = 8;
    const vec2* JitterOffsets = JitterOffsets8;

    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Accumulated);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.AccumulatedColor);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    for (int i = 0; i < JitterCount; i++) {
        
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Scene);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.SceneColor);

        RenderPass(objectTheta, fboTheta, JitterOffsets[i]);
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        const float NearPlane = 5, FarPlane = 50;
        glFrustumf(-0.5, 0.5, -0.5, 0.5, NearPlane, FarPlane);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -NearPlane * 2);
        
        float f = 1.0f / JitterCount;
        f *= (1 + abs(sin(fboTheta * Pi / 180)));
        glColor4f(f, f, f, 1);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); 
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Accumulated);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.AccumulatedColor);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, m_textures.OffscreenSurface);
        RenderDrawable(m_drawables.Quad);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }
}
    
void RenderingEngine::RenderPass(float objectTheta, float fboTheta, vec2 offset) const
{
    // Tweak the jitter offset for the defocus effect:
    
    offset -= vec2(0.5, 0.5);
    offset *= 1 + 100 * sin(fboTheta * Pi / 180);

    // Set up the frustum planes:

    const float AspectRatio = (float) m_viewport.y / m_viewport.x;
    const float NearPlane = 5;
    const float FarPlane = 50;
    const float LeftPlane = -1;
    const float RightPlane = 1;
    const float TopPlane = -AspectRatio;
    const float BottomPlane = AspectRatio;

    // Transform the jitter offset from window space to eye space:
    
    offset.x *= (RightPlane - LeftPlane) / m_viewport.x;
    offset.y *= (BottomPlane - TopPlane) / m_viewport.y;
    
    // Compute the jittered projection matrix:

    mat4 projection = mat4::Frustum(LeftPlane + offset.x, RightPlane + offset.x, 
                                    TopPlane + offset.y, BottomPlane + offset.y,
                                    NearPlane, FarPlane);
    
    // Look at fboTheta to determine which "side" should be rendered:
    //   1) Orange Trefoil Knot against a Tiger background
    //   2) Green Klein bottle against a Rhino background
    
    Drawable drawable;
    GLuint background;
    vec3 color;
    
    if (fboTheta > 270 || fboTheta < 90) {
        background = m_textures.TigerBackground;
        drawable = m_drawables.Knot;
        color = vec3(1, 1, 1);
    } else {
        background = m_textures.RhinoBackground;
        drawable = m_drawables.Bottle;
        objectTheta = -objectTheta;
        color = vec3(1, 1, 1);
    }
    
    // Set up some OpenGL state for the 3D scene.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.Pointer());
    glMatrixMode(GL_MODELVIEW);
    
    // Clear the back buffer and orient the scene.
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, -10);
    glRotatef(20, 1, 0, 0);
    
    // Prepare the render state for the reflection.
    glRotatef(objectTheta, 0, 1, 0);
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, vec4(color, 0).Pointer());

    // Render the floating object.
    RenderDrawable(drawable);

    // Prepare for rendering the disk.
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glRotatef(-objectTheta, 0, 1, 0);

    // Render the background.
    glDepthRangef(0.9, 1);
    glColor4f(0.7, 0.7, 0.7, 1);
    glBindTexture(GL_TEXTURE_2D, background);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -NearPlane);
    glScalef(1, 1.5, 1);
    glScalef(1.2, 1.2, 1);
    RenderDrawable(m_drawables.Quad);
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glDepthRangef(0, 1);
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