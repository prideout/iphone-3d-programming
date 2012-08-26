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
    Drawable Quad;
    Drawable UnitQuad;
};

struct Framebuffers {
    GLuint Screen;
    GLuint Page1;
    GLuint Page2;
};

struct Renderbuffers {
    GLuint Screen;
    GLuint Page1;
    GLuint Page2;
};

struct Textures {
    GLuint OldPaper;
    GLuint Page1;
    GLuint Page2;
};
    
class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize();
    void Render(const string& poem1,
                const string& poem2,
                float pageCurl) const;
private:
    void CreatePageFbo(GLuint* rb, GLuint* fb, GLuint* tex);
    GLuint CreateTexture(const string& file, bool npot);
    void UploadTexture(const TextureDescription& desc, const unsigned long* data);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    Drawables m_drawables;
    ivec2 m_pageSize;
    ivec2 m_pageSubsize;
    ivec2 m_screenSize;
    ITextRenderer* m_textRenderer;
    IResourceManager* m_resourceManager;
};

IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    m_textRenderer = ES1::CreateTextRenderer();
    
    glGenRenderbuffersOES(1, &m_renderbuffers.Screen);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
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
    m_pageSize = ivec2(512, 128);
    m_pageSubsize = ivec2(400, 100);

    PrettyPrintExtensions();
    m_textRenderer->Initialize();
    
    // Create vertex buffer objects.
    m_drawables.Quad = CreateDrawable(Quad(m_pageSubsize));
    m_drawables.UnitQuad = CreateDrawable(Quad(vec2(2, 2)));
    
    // Load up some textures.
    m_textures.OldPaper = CreateTexture("Paper.pvr", true);

    // Extract width and height from the color buffer.
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &m_screenSize.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &m_screenSize.y);
    
    // Create the on-screen FBO.
    glGenFramebuffersOES(1, &m_framebuffers.Screen);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    
    // Create the off-screen FBOs.
    CreatePageFbo(&m_renderbuffers.Page1, &m_framebuffers.Page1, &m_textures.Page1);
    CreatePageFbo(&m_renderbuffers.Page2, &m_framebuffers.Page2, &m_textures.Page2);
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHT0);
}
    
void RenderingEngine::CreatePageFbo(GLuint* renderbuffer,
                                    GLuint* framebuffer,
                                    GLuint* texture)
{
    int width = m_pageSize.x;
    int height = m_pageSize.y;

    glGenRenderbuffersOES(1, renderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, *renderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES, width, height);
    
    glGenFramebuffersOES(1, framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, *framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, *renderbuffer);
    
    // Create a texture object and associate it with the FBO.
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                              GL_TEXTURE_2D, *texture, 0);
    
    // check FBO status
    GLenum status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    if (status != GL_FRAMEBUFFER_COMPLETE_OES) {
        cout << "Incomplete FBO" << endl;
        exit(1);
    }
}

void RenderingEngine::Render(const string& poem1,
                             const string& poem2,
                             float pageCurl) const
{
    glViewport(0, 0, m_pageSubsize.x, m_pageSubsize.y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ivec2 textPosition(30, 70);

    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Page1);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Page1);
    glClearColor(0.8, 0.8, 0.4f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the background on Page 1:
    glBindTexture(GL_TEXTURE_2D, m_textures.OldPaper);
    glColor4f(1, 1, 1, 1);
    RenderDrawable(m_drawables.UnitQuad);
    
    // Render the text on Page 1:
    glColor4f(0, 0, 0, 1);
    m_textRenderer->RenderText(textPosition, poem1);
    
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Page2);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Page2);
    glClearColor(0.8, 0.4, 0.4f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the background on Page 2:
    glBindTexture(GL_TEXTURE_2D, m_textures.OldPaper);
    glColor4f(1, 1, 1, 1);
    RenderDrawable(m_drawables.UnitQuad);
    
    // Render the text on Page 2:
    glColor4f(0, 0, 0, 1);
    m_textRenderer->RenderText(textPosition, poem2);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    const float NearPlane = m_pageSize.x, FarPlane = m_pageSize.x * 3;
    const float Scale = 0.5;
    glFrustumf(-Scale * m_screenSize.x / 2, Scale * m_screenSize.x / 2,
               -Scale * m_screenSize.y / 2, Scale * m_screenSize.y / 2,
               NearPlane, FarPlane);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(270, 0, 0, 1);
    glTranslatef(0, 0, -m_pageSize.x * 2);

    glViewport(0, 0, m_screenSize.x, m_screenSize.y);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Set the texture matrix to addess a NPOT region of the page:
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef((float) m_pageSubsize.x / m_pageSize.x,
             (float) m_pageSubsize.y / m_pageSize.y,
             1);

    // Draw a textured quad for Page 2:
    glEnable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, m_textures.Page2);
    RenderDrawable(m_drawables.Quad);

    // Hinge the rotation on the left edge:
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(-m_pageSubsize.x / 2, 0, 0);
    glRotatef(-90 * pageCurl, 0, 1, 0);
    glTranslatef(m_pageSubsize.x / 2, 0, 0);

    // Draw a textured quad for Page 1:
    glBindTexture(GL_TEXTURE_2D, m_textures.Page1);
    RenderDrawable(m_drawables.Quad);
    
    // Reset the texture matrix:
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
}

void RenderingEngine::UploadTexture(const TextureDescription& desc,
                                    const unsigned long* data)
{
    GLenum type;
    GLenum format;
    int bitsPerPixel;
    bool compressed = false;
    
    switch (desc.Format) {
        case TextureFormatPvrtcRgba4:
            compressed = true;
            format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            bitsPerPixel = 4;
            break;
        case TextureFormatPvrtcRgb4:
            compressed = true;
            format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            bitsPerPixel = 4;
            break;
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
        case TextureFormatGray:
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 8;
            break;
    }
    
    int w = desc.Size.x;
    int h = desc.Size.y;
    
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
        break;
    }
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

    TextureDescription desc = m_resourceManager->LoadImage(file);
    unsigned long* data = (unsigned long*) m_resourceManager->GetImageData();
    UploadTexture(desc, data);
    m_resourceManager->UnloadImage();
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

}