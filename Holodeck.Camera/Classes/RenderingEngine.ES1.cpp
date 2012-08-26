#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include "ParametricSurface.hpp"
#include "../Models/GeodesicDome.h"

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
    int VertexCount;
};

struct Drawables {
    Drawable GeodesicDome;
    Drawable SkySphere;
    Drawable Quad;
};

struct Textures {
    GLuint Sky;
    GLuint Floor;
    GLuint Button;
    GLuint Triangle;
    GLuint North;
    GLuint South;
    GLuint East;
    GLuint West;
};

struct Renderbuffers {
    GLuint Color;
    GLuint Depth;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager* resourceManager);
    void Initialize(bool opaqueBackground);
    void Render(float theta, float phi, ButtonMask buttons) const;
private:
    void RenderText(GLuint texture, float theta, float scale) const;
    GLuint CreateTexture(const string& file);
    Drawable CreateDrawable(const ParametricSurface& surface);
    Drawable CreateDrawable(const float* vertices, int VertexCount);
    void RenderDrawable(const Drawable& drawable) const;
    void UploadImage(const TextureDescription& description);
    void SetButtonAlpha(ButtonMask pressed, ButtonFlags flag) const;
    bool m_opaqueBackground;
    Drawables m_drawables;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    IResourceManager* m_resourceManager;
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

void RenderingEngine::Initialize(bool opaqueBackground)
{
    m_opaqueBackground = opaqueBackground;
    
    // Create vertex buffer objects.
    m_drawables.GeodesicDome = CreateDrawable(DomeVertices, DomeVertexCount);
    m_drawables.SkySphere = CreateDrawable(Sphere(1));
    m_drawables.Quad = CreateDrawable(Quad(64));
    
    // Load up some textures.
    m_textures.Floor = CreateTexture("Moss.pvr");
    m_textures.Sky = CreateTexture("Sky.pvr");
    m_textures.Button = CreateTexture("Button.png");
    m_textures.Triangle = CreateTexture("Triangle.png");
    m_textures.North = CreateTexture("North.png");
    m_textures.South = CreateTexture("South.png");
    m_textures.East = CreateTexture("East.png");
    m_textures.West = CreateTexture("West.png");

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
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
        
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
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set the model-view transform.
    glMatrixMode(GL_MODELVIEW);
    glRotatef(90, 0, 0, 1);
    
    // Set the projection transform.
    float h = 4.0f * height / width;
    glMatrixMode(GL_PROJECTION);
    glFrustumf(-2, 2, -h / 2, h / 2, 5, 200);
    glMatrixMode(GL_MODELVIEW);
}

void RenderingEngine::RenderText(GLuint texture, float theta, float scale) const
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glPushMatrix();
    glRotatef(theta, 0, 1, 0);
    glTranslatef(0, -1, -30);
    glScalef(-2 * scale, -scale, scale);
    RenderDrawable(m_drawables.Quad);
    glPopMatrix();
}

void RenderingEngine::Render(float theta, float phi, ButtonMask buttons) const
{
    static float frameCounter = 0;
    frameCounter++;
    
    glPushMatrix();

    glRotatef(phi, 1, 0, 0);
    glRotatef(theta, 0, 1, 0);

    if (m_opaqueBackground) {
        glClear(GL_DEPTH_BUFFER_BIT);

        glPushMatrix();
        glScalef(100, 100, 100);
        glRotatef(frameCounter * 2, 0, 1, 0);
        glBindTexture(GL_TEXTURE_2D, m_textures.Sky);
        RenderDrawable(m_drawables.SkySphere);
        glPopMatrix();
    } else {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Triangle);
    glPushMatrix();
    glTranslatef(0, 10, 0);
    glScalef(90, 90, 90);
    RenderDrawable(m_drawables.GeodesicDome);
    glPopMatrix();
    
    float textScale = 1.0 / 10.0 + sin(frameCounter / 10.0f) / 150.0;
    
    RenderText(m_textures.East, 0, textScale);
    RenderText(m_textures.West, 180, textScale);
    RenderText(m_textures.South, 90, textScale);
    RenderText(m_textures.North, -90, textScale);
    glDisable(GL_BLEND);

    glTranslatef(0, 10, -10);
    glRotatef(90, 1, 0, 0);
    glScalef(4, 4, 4);
    glMatrixMode(GL_TEXTURE);
    glScalef(4, 4, 1);
    glBindTexture(GL_TEXTURE_2D, m_textures.Floor);
    RenderDrawable(m_drawables.Quad);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    if (buttons) {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, m_textures.Button);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
            glLoadIdentity();
            glOrthof(-160, 160, -240, 240, 0, 1);

            if (buttons & ButtonFlagsShowHorizontal) {
                glMatrixMode(GL_MODELVIEW);
                glTranslatef(200, 0, 0);
                SetButtonAlpha(buttons, ButtonFlagsPressingLeft);
                RenderDrawable(m_drawables.Quad);
                glTranslatef(-400, 0, 0);
                glMatrixMode(GL_TEXTURE);
                glRotatef(180, 0, 0, 1);
                SetButtonAlpha(buttons, ButtonFlagsPressingRight);
                RenderDrawable(m_drawables.Quad);
                glRotatef(-180, 0, 0, 1);
                glMatrixMode(GL_MODELVIEW); 
                glTranslatef(200, 0, 0);
            }
        
            if (buttons & ButtonFlagsShowVertical) {
                glMatrixMode(GL_MODELVIEW);
                glTranslatef(0, 125, 0);
                glMatrixMode(GL_TEXTURE);
                glRotatef(90, 0, 0, 1);
                SetButtonAlpha(buttons, ButtonFlagsPressingUp);
                RenderDrawable(m_drawables.Quad);
                glMatrixMode(GL_MODELVIEW);
                glTranslatef(0, -250, 0);
                glMatrixMode(GL_TEXTURE);
                glRotatef(180, 0, 0, 1);
                SetButtonAlpha(buttons, ButtonFlagsPressingDown);
                RenderDrawable(m_drawables.Quad);
                glRotatef(90, 0, 0, 1);
                glMatrixMode(GL_MODELVIEW);
                glTranslatef(0, 125, 0);
            }
        
            glColor4f(1, 1, 1, 1);
            glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
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
    unsigned char vertexFlags = VertexFlagsTexCoords;
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
    drawable.VertexCount = vertices.size();
    return drawable;
}

Drawable RenderingEngine::CreateDrawable(const float* vertices, int vertexCount)
{
    // Create the VBO for the vertices.
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexCount * 5 * sizeof(*vertices),
                 vertices,
                 GL_STATIC_DRAW);
    
    // Fill in a descriptive struct and return it.
    Drawable drawable;
    drawable.IndexBuffer = 0;
    drawable.VertexBuffer = vertexBuffer;
    drawable.IndexCount = 0;
    drawable.VertexCount = vertexCount;
    return drawable;
}

void RenderingEngine::RenderDrawable(const Drawable& drawable) const
{
    int stride = sizeof(vec3) + sizeof(vec2);
    const GLvoid* texCoordOffset = (const GLvoid*) sizeof(vec3);
    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
    
    if (drawable.IndexBuffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, drawable.VertexCount);
    }
}

void RenderingEngine::SetButtonAlpha(ButtonMask pressed, ButtonFlags flag) const
{
    float alpha = (pressed & flag) ? 1.0 : 0.75;
    glColor4f(1, 1, 1, alpha);
}

void RenderingEngine::UploadImage(const TextureDescription& desc)
{
    GLenum type;
    GLenum format;
    int bitsPerPixel;
    bool compressed = false;
    bool mipped = false;
    
    switch (desc.Format) {
        case TextureFormatPvrtcRgba2:
            compressed = true;
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            mipped = true;
            break;
        case TextureFormatPvrtcRgb2:
            compressed = true;
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            mipped = true;
            break;
        case TextureFormat565:
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            bitsPerPixel = 16;
            mipped = true;
            break;
        case TextureFormatGray:
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 8;
            mipped = true;
            break;
        case TextureFormatGrayAlpha:
            format = GL_LUMINANCE_ALPHA;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 16;
            mipped = true;
            break;
        case TextureFormatRgb:
            format = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 24;
            mipped = true;
            break;
        case TextureFormatRgba:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            bitsPerPixel = 32;
            mipped = false;
            break;
    }
    
    int w = desc.Size.x;
    int h = desc.Size.y;
    unsigned char* data = (unsigned char*) m_resourceManager->GetImageData();
    
    int level = 0;
    while (w && h) {
        GLsizei size = w * h * bitsPerPixel / 8;
        
        if (compressed) {
            size = size < 32 ? 32 : size;
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, size, data);
        }
        else
            glTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, format, type, data);
        
        if (level < desc.MipCount - 1)
            data += size;
        
        if (!mipped)
            break;
        
        w >>= 1; h >>= 1; level++;
    }
    
    m_resourceManager->UnloadImage();
}
