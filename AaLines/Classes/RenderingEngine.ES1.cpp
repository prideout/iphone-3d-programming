#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include "../Textures/BlurryCircle.h"
#include "../Textures/Circle.h"
#include "../Textures/Tile.h"
#include <vector>

const int JointCount = 18;
extern const float ShortMoCap[21][JointCount][3];
extern const float LongMoCap[653][JointCount][3];
extern const unsigned short StickFigureIndices[17 * 2];

using namespace std;

namespace ES1 {

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
    DemoStateAaLines,
    DemoStateNormalLines,
    DemoStateWideLines,
    DemoStateCount
};
    
struct Framebuffers {
    GLuint Screen;
};

struct Renderbuffers {
    GLuint Screen;
};

struct Textures {
    GLuint Tile;
    GLuint Circle;
    GLuint BlurryCircle;
};
    
struct Vertex {
    vec3 Position;
    vec2 TexCoord;
};

typedef std::vector<Vertex> VertexList;
typedef std::vector<GLushort> IndexList;
    
struct StickFigure {
    GLuint IndexBuffer;
    IndexList Indices;
    VertexList Vertices;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render() const;
    void UpdateAnimation(float timestamp);
private:
    GLuint CreateTexture(const unsigned long* data);
    
    void GenerateTriangleIndices(size_t lineCount, IndexList& triangles) const;
    void GenerateTriangleTexCoords(StickFigure& triangles) const;
    
    void ExtrudeLines(const StickFigure& lines,
                      StickFigure& triangles,
                      float width) const;
    
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    VertexList m_backgroundVertices;
    StickFigure m_stickFigure;
    StickFigure m_aaStickFigure;
    DemoState m_demoState;
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
    // Create vertices for full-screen quad.
    m_backgroundVertices.resize(4);
    m_backgroundVertices[0].Position = vec3(-1, -1.5, 0);
    m_backgroundVertices[0].TexCoord = vec2(0, 0);
    m_backgroundVertices[1].Position = vec3(-1, 1.5, 0);
    m_backgroundVertices[1].TexCoord = vec2(0, 1);
    m_backgroundVertices[2].Position = vec3(1, -1.5, 0);
    m_backgroundVertices[2].TexCoord = vec2(1, 0);
    m_backgroundVertices[3].Position = vec3(1, 1.5, 0);
    m_backgroundVertices[3].TexCoord = vec2(1, 1);

    if (false) {
        m_backgroundVertices[0].TexCoord *= 0.3;
        m_backgroundVertices[1].TexCoord *= 0.3;
        m_backgroundVertices[2].TexCoord *= 0.3;
        m_backgroundVertices[3].TexCoord *= 0.3;
    }
    
    // Create the line-based stick figure.
    size_t indexCount = sizeof(StickFigureIndices) / sizeof(GLushort);
    size_t lineCount = indexCount / 2;
    m_stickFigure.Indices = IndexList(StickFigureIndices, StickFigureIndices + indexCount);
    glGenBuffers(1, &m_stickFigure.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_stickFigure.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_stickFigure.Indices.size() * sizeof(GLushort),
                 &m_stickFigure.Indices[0],
                 GL_STATIC_DRAW);
    m_stickFigure.Vertices.resize(JointCount);
    
    // Create the triangle-based stick figure.
    GenerateTriangleIndices(lineCount, m_aaStickFigure.Indices);
    glGenBuffers(1, &m_aaStickFigure.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_aaStickFigure.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_aaStickFigure.Indices.size() * sizeof(GLushort),
                 &m_aaStickFigure.Indices[0],
                 GL_STATIC_DRAW);
    m_aaStickFigure.Vertices.resize(lineCount * 8);
    GenerateTriangleTexCoords(m_aaStickFigure);

    // Initialize the demo state.
    m_demoState = DemoStateAaLines;
    
    // Load up some textures.
    m_textures.Tile = CreateTexture(Tile);
    m_textures.Circle = CreateTexture(Circle);
    m_textures.BlurryCircle = CreateTexture(BlurryCircle);

    // Extract width and height from the color buffer.
    ivec2 screenSize;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &screenSize.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &screenSize.y);
    
    // Create the on-screen FBO.
    glGenFramebuffersOES(1, &m_framebuffers.Screen);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, screenSize.x, screenSize.y);
    glAlphaFunc(GL_LESS, 0.5);
    
    // Set up the transforms.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    const float NearPlane = 5, FarPlane = 100;
    const float Scale = 0.0005;
    glFrustumf(-Scale * screenSize.x / 2, Scale * screenSize.x / 2,
               -Scale * screenSize.y / 2, Scale * screenSize.y / 2,
               NearPlane, FarPlane);
    
    glMatrixMode(GL_MODELVIEW);
}
    
void RenderingEngine::UpdateAnimation(float timestamp)
{
    int timeIndex = (int) (timestamp * 40.0f);
    
    //timeIndex = 63 + 57 + 653 + 653;
    
    int shortFrameCount = sizeof(ShortMoCap) / sizeof(ShortMoCap[0]);
    int longFrameCount = sizeof(LongMoCap) / sizeof(LongMoCap[0]);
    int shortLength = shortFrameCount * (int) DemoStateCount;
    int longLength = longFrameCount * (int) DemoStateCount;
    bool longDemo = (timeIndex % (shortLength + longLength)) >= shortLength;
    int frameCount = longDemo ? longFrameCount : shortFrameCount;

    if (longDemo)
        timeIndex -= shortLength;

    int frameIndex = timeIndex % frameCount;
    int demoState = (timeIndex / frameCount) % DemoStateCount;
    m_demoState = (DemoState) demoState;
    
    for (int i = 0; i < JointCount; i++) {
        if (longDemo) {
            m_stickFigure.Vertices[i].Position.x = LongMoCap[frameIndex][i][0];
            m_stickFigure.Vertices[i].Position.y = LongMoCap[frameIndex][i][1];
        } else {
            m_stickFigure.Vertices[i].Position.x = ShortMoCap[frameIndex][i][0];
            m_stickFigure.Vertices[i].Position.y = ShortMoCap[frameIndex][i][1];
        }
        m_stickFigure.Vertices[i].Position.z = 0;
    }
    
    if (m_demoState != DemoStateNormalLines) {
        const StickFigure& lines = m_stickFigure;
        float width = m_demoState == DemoStateAaLines ? 0.01 : 0.03;
        // width /= 2;
        StickFigure& triangles = m_aaStickFigure;
        ExtrudeLines(lines, triangles, width);
    }
}

void RenderingEngine::Render() const
{
    GLsizei stride = sizeof(Vertex);

    // Set up camera:
    //vec3 eye(0, 0, 40);
    vec3 eye(0, 0, 62);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);
    mat4 modelview = mat4::LookAt(eye, target, up);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(modelview.Pointer());

    // Draw background:
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tile);
    glVertexPointer(3, GL_FLOAT, stride, &m_backgroundVertices[0].Position);
    glTexCoordPointer(2, GL_FLOAT, stride, &m_backgroundVertices[0].TexCoord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Bind the correct texture:
    switch (m_demoState) {
        case DemoStateAaLines:
            glBindTexture(GL_TEXTURE_2D, m_textures.BlurryCircle);
            break;
        case DemoStateWideLines:
            glBindTexture(GL_TEXTURE_2D, m_textures.Circle);
            break;
    }

    // Render the stick figure:
    glTranslatef(-0.25, -1.25, 0);
    //glTranslatef(0, -0.85, 0);
    glColor4f(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    switch (m_demoState) {
        case DemoStateNormalLines:
        {
            const StickFigure& lines = m_stickFigure;
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisable(GL_TEXTURE_2D);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lines.IndexBuffer);
            glVertexPointer(3, GL_FLOAT, stride, &lines.Vertices[0].Position);
            glDrawElements(GL_LINES, lines.Indices.size(), GL_UNSIGNED_SHORT, 0);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnable(GL_TEXTURE_2D);
            break;
        }
            
        case DemoStateAaLines:
        case DemoStateWideLines:
        {
            const StickFigure& triangles = m_aaStickFigure;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangles.IndexBuffer);
            glVertexPointer(3, GL_FLOAT, stride, &triangles.Vertices[0].Position);
            glTexCoordPointer(2, GL_FLOAT, stride, &triangles.Vertices[0].TexCoord);
            glDrawElements(GL_TRIANGLES, triangles.Indices.size(), GL_UNSIGNED_SHORT, 0);
            break;
        }
            
    }
}

void RenderingEngine::ExtrudeLines(const StickFigure& lines,
                                   StickFigure& triangles,
                                   float width) const
{
    size_t lineIndex = 0;
    Vertex* destVertex = &triangles.Vertices[0];
    while (lineIndex < lines.Indices.size()) {
        
        vec3 a = lines.Vertices[lines.Indices[lineIndex++]].Position;
        vec3 b = lines.Vertices[lines.Indices[lineIndex++]].Position;
        
        vec3 E = (b - a).Normalized() * width;
        vec3 N = vec3(-E.y, E.x, 0);
        vec3 S = -N;
        vec3 NE = N + E;
        vec3 NW = N - E;
        vec3 SW = -NE;
        vec3 SE = -NW;
        
        destVertex++->Position = a + SW;
        destVertex++->Position = a + NW;
        destVertex++->Position = a + S;
        destVertex++->Position = a + N;
        destVertex++->Position = b + S;
        destVertex++->Position = b + N;
        destVertex++->Position = b + SE;
        destVertex++->Position = b + NE;
    }
}
    
void RenderingEngine::GenerateTriangleIndices(size_t lineCount,
                                             IndexList& triangles) const
{
    size_t destVertCount = lineCount * 8;
    triangles.resize(lineCount * 18);
    
    IndexList::iterator triangle = triangles.begin();
    
    for (GLushort v = 0; v < destVertCount; v += 8) {
        *triangle++ = 0 + v; *triangle++ = 1 + v; *triangle++ = 2 + v;
        *triangle++ = 2 + v; *triangle++ = 1 + v; *triangle++ = 3 + v;
        
        *triangle++ = 2 + v; *triangle++ = 3 + v; *triangle++ = 4 + v;
        *triangle++ = 4 + v; *triangle++ = 3 + v; *triangle++ = 5 + v;
        
        *triangle++ = 4 + v; *triangle++ = 5 + v; *triangle++ = 6 + v;
        *triangle++ = 6 + v; *triangle++ = 5 + v; *triangle++ = 7 + v;
    }
}

void RenderingEngine::GenerateTriangleTexCoords(StickFigure& triangles) const
{
    Vertex* destVertex = &triangles.Vertices[0];
    for (size_t i = 0; i < triangles.Vertices.size(); i += 8) {
        destVertex++->TexCoord = vec2(0, 0);
        destVertex++->TexCoord = vec2(0, 1);
        destVertex++->TexCoord = vec2(0.5, 0);
        destVertex++->TexCoord = vec2(0.5, 1);
        destVertex++->TexCoord = vec2(0.5, 0);
        destVertex++->TexCoord = vec2(0.5, 1);
        destVertex++->TexCoord = vec2(1, 0);
        destVertex++->TexCoord = vec2(1, 1);
    }
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
    
}