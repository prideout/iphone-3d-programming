#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include "../Textures/DistanceField.h"
#include "../Textures/Tile.h"
#include "../Textures/Text.h"
#include "../Textures/SmallAum.h"

using namespace std;

namespace ES1 {

const vec3 CameraPositions[] = {
    vec3(0, 0, 40),
    vec3(20, 0, 40),
    vec3(20, 0, 23),
    vec3(20, 0, 6),
    vec3(15, 0, 6),
    vec3(0, 0, 6),
    vec3(0, 0, 23),
    vec3(0, 0, 40),
};

const int NumCameraPositions = sizeof(CameraPositions) / sizeof(vec3);
const float CameraSpeed = 5;

struct PVRTextureHeader {
    unsigned int HeaderSize;
    unsigned int Height;
    unsigned int Width;
    unsigned int MipMapCount;
    unsigned int Flags;
    unsigned int TextureDataSize;
    unsigned int BitCount;
    unsigned int RBitMask;
    unsigned int GBitMask;
    unsigned int BBitMask;
    unsigned int AlphaBitMask;
    unsigned int PVR;
    unsigned int NumSurfs;
};

enum PVRPixelType {
    OGL_RGBA_4444 = 0x10,
    OGL_RGBA_5551,
    OGL_RGBA_8888,
    OGL_RGB_565,
    OGL_RGB_555,
    OGL_RGB_888,
    OGL_I_8,
    OGL_AI_88,
    OGL_PVRTC2,
    OGL_PVRTC4,
    PVRTEX_PIXELTYPE = 0xff,
};

enum DemoState {
    DemoStateAlphaBlended,
    DemoStateAlphaTested,
    DemoStateDistanceField,
    DemoStateCount
};
    
struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

struct Drawables {
    Drawable Glyph;
    Drawable Square;
    Drawable Label;
};

struct Framebuffers {
    GLuint Screen;
};

struct Renderbuffers {
    GLuint Screen;
};

struct Textures {
    GLuint DistanceField;
    GLuint SmallAum;
    GLuint Tile;
    GLuint Text;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(float timestamp) const;
private:
    GLuint CreateTexture(const unsigned long* data);
    Drawable CreateDrawable(const ParametricSurface& surface);
    void RenderDrawable(const Drawable& drawable) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    Drawables m_drawables;
    ivec2 m_screenSize;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine()
{
    glGenRenderbuffersOES(1, &m_renderbuffers.Screen);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
}

void RenderingEngine::Initialize()
{
    // Create vertex buffer objects.
    m_drawables.Glyph = CreateDrawable(Quad(2, 1));
    m_drawables.Square = CreateDrawable(Quad(3, 3));
    m_drawables.Label = CreateDrawable(Quad(0.75f, 0.125f));
    
    // Load up some textures.
    m_textures.DistanceField = CreateTexture(DistanceField);
    m_textures.SmallAum = CreateTexture(SmallAum);
    m_textures.Tile = CreateTexture(Tile);
    m_textures.Text = CreateTexture(Text);

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
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, m_screenSize.x, m_screenSize.y);
    glAlphaFunc(GL_LESS, 0.5);
    
    // Set up the transforms.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    const float NearPlane = 5, FarPlane = 100;
    const float Scale = 0.0005;
    glFrustumf(-Scale * m_screenSize.x / 2, Scale * m_screenSize.x / 2,
               -Scale * m_screenSize.y / 2, Scale * m_screenSize.y / 2,
               NearPlane, FarPlane);
    
    glMatrixMode(GL_MODELVIEW);
}
    
void RenderingEngine::Render(float timestamp) const
{
    if (false) {
        int desiredState = 0;
        float desiredSegment = 5.5 + desiredState * NumCameraPositions;
        timestamp = desiredSegment * NumCameraPositions / CameraSpeed;
    }
    
    float segment = timestamp * CameraSpeed / NumCameraPositions;
    int integer = (int) segment;
    float fract = segment - integer;
    vec3 from = CameraPositions[integer % NumCameraPositions];
    vec3 to = CameraPositions[(integer + 1) % NumCameraPositions];
    
    vec3 eye = from.Lerp(fract, to);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);

    mat4 modelview = mat4::LookAt(eye, target, up);
    glLoadMatrixf(modelview.Pointer());

    glColor4f(1, 1, 1, 1);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tile);
    RenderDrawable(m_drawables.Square);

    glColor4f(0, 0, 0, 1);
    DemoState state = (DemoState) ((integer / NumCameraPositions) % DemoStateCount);
    switch (state) {
        case DemoStateAlphaBlended:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glBindTexture(GL_TEXTURE_2D, m_textures.SmallAum);
            RenderDrawable(m_drawables.Glyph);
            glDisable(GL_BLEND);
            break;
        case DemoStateAlphaTested:
            glEnable(GL_ALPHA_TEST);
            glBindTexture(GL_TEXTURE_2D, m_textures.SmallAum);
            RenderDrawable(m_drawables.Glyph);
            glDisable(GL_ALPHA_TEST);
            break;
        case DemoStateDistanceField:
            glEnable(GL_ALPHA_TEST);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
            glDisable(GL_ALPHA_TEST);
            break;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, m_textures.Text);

    modelview = mat4::LookAt(vec3(0, -0.5f, 25), vec3(0, -0.5f, 0), up);
    glLoadMatrixf(modelview.Pointer());
    
    glMatrixMode(GL_TEXTURE);
    glScalef(1, 0.33, 1);

    switch (state) {
        case DemoStateAlphaBlended:
            glTranslatef(0, -1, 0);
            break;
        case DemoStateAlphaTested:
            glTranslatef(0, 1, 0);
            break;
        case DemoStateDistanceField:
            break;
    }
    
    RenderDrawable(m_drawables.Label);
    glDisable(GL_BLEND);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

GLuint RenderingEngine::CreateTexture(const unsigned long* data)
{
    PVRTextureHeader* header = (PVRTextureHeader*) data;
    GLsizei w = header->Width;
    GLsizei h = header->Height;
    const unsigned long* texels = data + header->HeaderSize / 4;

    GLuint name;
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    switch (header->Flags & PVRTEX_PIXELTYPE) {
        case OGL_I_8: {
            GLenum format = GL_ALPHA;
            GLenum type = GL_UNSIGNED_BYTE;
            glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, texels);
            break;
        }
        case OGL_RGB_565: {
            GLenum format = GL_RGB;
            GLenum type = GL_UNSIGNED_SHORT_5_6_5;
            glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, texels);
            break;
        }
        case OGL_PVRTC4: {
            GLsizei size = max(32, w * h / 2);
            GLenum format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, size, data);
            break;
        }
        default:
            printf("Unknown format.\n");
    }

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
    return drawable;
}

void RenderingEngine::RenderDrawable(const Drawable& drawable) const
{
    int stride = sizeof(vec3) + sizeof(vec2);
    const GLvoid* texCoordOffset = (const GLvoid*) sizeof(vec3);
    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
}

}