#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include <iostream>
#include <assert.h>

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
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
    void SetPngTexture(const string& name) const;
    void SetPvrTexture(const string& file) const;
private:
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    mat4 m_translation;
    IResourceManager* m_resourceManager;
    mutable float m_offset;
};
    
IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
    m_offset = 0;
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void RenderingEngine::Initialize(const vector<ISurface*>& surfaces)
{
    vector<ISurface*>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
        
        // Create the VBO for the vertices.
        vector<float> vertices;
        unsigned char vertexFlags = VertexFlagsTexCoords;
        (*surface)->GenerateVertices(vertices, vertexFlags);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(vertices[0]),
                     &vertices[0],
                     GL_STATIC_DRAW);
        
        // Create a new VBO for the indices if needed.
        int indexCount = (*surface)->GetTriangleIndexCount();
        GLuint indexBuffer;
        if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
            indexBuffer = m_drawables[0].IndexBuffer;
        } else {
            vector<GLushort> indices(indexCount);
            (*surface)->GenerateTriangleIndices(indices);
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         indexCount * sizeof(GLushort),
                         &indices[0],
                         GL_STATIC_DRAW);
        }
        
        Drawable drawable = { vertexBuffer, indexBuffer, indexCount};
        m_drawables.push_back(drawable);
    }

    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);

    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES,
                             width, height);

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
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up the texture state.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Set up the material properties.
    vec4 specular(0.5f, 0.5f, 0.5f, 1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular.Pointer());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);

    m_translation = mat4::Translate(0, 0, -7);
}

void RenderingEngine::Render(const vector<Visual>& visuals) const
{
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {
        
        // Set the viewport transform.
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
        
        // Set the light position.
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Set the model-view transform.
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelview = rotation * m_translation;
        glLoadMatrixf(modelview.Pointer());
        
        // Set the projection transform.
        float h = 4.0f * size.y / size.x;
        mat4 projection = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.Pointer());
        
        // Set the diffuse color.
        vec3 color = visual->Color * 0.75f;
        vec4 diffuse(color.x, color.y, color.z, 1);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.Pointer());

        // Draw the surface.
        int stride = sizeof(vec3) +  sizeof(vec2);
        const GLvoid* texCoordOffset = (const GLvoid*) sizeof(vec3);
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexPointer(3, GL_FLOAT, stride, 0);
        glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
}

void RenderingEngine::SetPngTexture(const string& filename) const
{
    TextureDescription description = m_resourceManager->LoadImage(filename);
    //TextureDescription description = m_resourceManager->GenerateCircle();
    
    GLenum format;
    switch (description.Format) {
        case TextureFormatGray:      format = GL_LUMINANCE;       break;
        case TextureFormatGrayAlpha: format = GL_LUMINANCE_ALPHA; break;
        case TextureFormatRgb:       format = GL_RGB;             break;
        case TextureFormatRgba:      format = GL_RGBA;            break;
    }
    
    GLenum type;
    switch (description.BitsPerComponent) {
        case 8: type = GL_UNSIGNED_BYTE; break;
        case 4:
            if (format == GL_RGBA) {
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            }
            // intentionally fall through
        default:
            assert(!"Unsupported format.");
    }
    
    void* data = m_resourceManager->GetImageData();
    ivec2 size = description.Size;
    glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, type, data);
    m_resourceManager->UnloadImage();
}

void RenderingEngine::SetPvrTexture(const string& filename) const
{
    TextureDescription description = m_resourceManager->LoadPvrImage(filename);
    unsigned char* data = (unsigned char*) m_resourceManager->GetImageData();
    int width = description.Size.x;
    int height = description.Size.y;
    
    int bitsPerPixel;
    GLenum format;
    bool compressed = true;
    switch (description.Format) {
        case TextureFormatPvrtcRgba2:
            bitsPerPixel = 2;
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgb2:
            bitsPerPixel = 2;
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgba4:
            bitsPerPixel = 4;
            format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgb4:
            bitsPerPixel = 4;
            format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            break;
        default:
            compressed = false;
            break;
    }
    
    if (compressed) {
        for (int level = 0; level < description.MipCount; ++level) {
            GLsizei size = max(32, width * height * bitsPerPixel / 8);
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, size, data);
            data += size;
            width >>= 1; height >>= 1;
        }
    } else {
        GLenum type;
        switch (description.Format) {
            case TextureFormatRgba:
                assert(description.BitsPerComponent == 4);
                format = GL_RGBA;
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                bitsPerPixel = 16;
                break;
            case TextureFormat565:
                format = GL_RGB;
                type = GL_UNSIGNED_SHORT_5_6_5;
                bitsPerPixel = 16;
                break;
            case TextureFormat5551:
                format = GL_RGBA;
                type = GL_UNSIGNED_SHORT_5_5_5_1;
                bitsPerPixel = 16;
                break;
        }
        for (int level = 0; level < description.MipCount; ++level) {
            glTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, format, type, data);
            GLsizei size = width * height * bitsPerPixel / 8;
            data += size;
            width >>= 1; height >>= 1;
        }
    }
    
    m_resourceManager->UnloadImage();
}

}