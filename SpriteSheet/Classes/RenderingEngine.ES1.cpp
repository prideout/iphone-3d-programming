#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Box.hpp"
#include "../Textures/Unified.h"
#include "../Textures/BodyLayer.h"
#include "../Textures/EyesLayer.h"
#include "../Textures/Tile.h"
#include "../Textures/Background.h"
#include <vector>

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
    
struct Framebuffers {
    GLuint Screen;
};

struct Renderbuffers {
    GLuint Screen;
};

struct Textures {
    GLuint Tile;
    GLuint Unified;
    GLuint Background;
    GLuint Body;
    GLuint Eyes;
};
    
struct Vertex {
    vec3 Position;
    vec2 TexCoord;
};
    
struct NoopSprite {
    box2 Body;
    box2 Eyes;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render() const;
    void UpdateAnimation(float timestamp);
private:
    GLuint CreateTexture(const unsigned long* data);
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    std::vector<NoopSprite> m_noopFrames;
    std::vector<NoopSprite> m_unifiedFrames;
    size_t m_frameCount;
    size_t m_frameIndex;
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
    // Load up the sprite coordinates:
    m_frameCount = sizeof(BodyLayerBoxes) / sizeof(box2);
    m_frameIndex = 0;
    m_noopFrames.resize(m_frameCount);
    m_unifiedFrames.resize(m_frameCount);

    const float* pBody = BodyLayerBoxes;
    const float* pEyes = EyeLayerBoxes;
    for (size_t i = 0; i < m_frameCount; ++i) {
        float l, t, r, b;
        
        l = *pBody++;
        t = *pBody++;
        r = *pBody++;
        b = *pBody++;
        m_noopFrames[i].Body = box2::FromLeftTopRightBottom(l, t, r, b);
        m_noopFrames[i].Body.FlipY(512);

        l = *pEyes++;
        t = *pEyes++;
        r = *pEyes++;
        b = *pEyes++;
        m_noopFrames[i].Eyes  = box2::FromLeftTopRightBottom(l, t, r, b);
        m_noopFrames[i].Eyes.FlipY(512);
    }
    
    pBody = UnifiedBodyLayerBoxes;
    pEyes = UnifiedEyeLayerBoxes;
    for (size_t i = 0; i < m_frameCount; ++i) {
        float l, t, r, b;
        
        l = *pBody++;
        t = *pBody++;
        r = *pBody++;
        b = *pBody++;
        m_unifiedFrames[i].Body = box2::FromLeftTopRightBottom(l, t, r, b);
        m_unifiedFrames[i].Body.FlipY(512);
        
        l = *pEyes++;
        t = *pEyes++;
        r = *pEyes++;
        b = *pEyes++;
        m_unifiedFrames[i].Eyes  = box2::FromLeftTopRightBottom(l, t, r, b);
        m_unifiedFrames[i].Eyes.FlipY(512);
    }
    
    // Load up some textures:
    m_textures.Tile = CreateTexture(Tile);
    m_textures.Eyes = CreateTexture(EyesLayer);
    m_textures.Body = CreateTexture(BodyLayer);
    m_textures.Unified = CreateTexture(Unified);
    m_textures.Background = CreateTexture(_Background_pvrtc);

    // Extract width and height from the color buffer:
    ivec2 screenSize;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &screenSize.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &screenSize.y);
    
    // Create the on-screen FBO:
    glGenFramebuffersOES(1, &m_framebuffers.Screen);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    
    // Set up various GL state:
    glViewport(0, 0, screenSize.x, screenSize.y);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up the second texture stage for typical alpha blending:
    glActiveTexture(GL_TEXTURE1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
}
    
void RenderingEngine::UpdateAnimation(float timestamp)
{
    float speed = 30;
    int timeindex = ((int) (timestamp * speed)) % (m_frameCount * 2);
    if (timeindex < m_frameCount)
    {
        m_frameIndex = timeindex;
    }
    else
    {
        timeindex -= m_frameCount;
        m_frameIndex = m_frameCount - timeindex - 1;
    }
}

void RenderingEngine::Render() const
{
    NoopSprite sprite = m_unifiedFrames[m_frameIndex];
    //NoopSprite sprite = m_noopFrames[m_frameIndex];

    float width = sprite.Body.width;
    float height = sprite.Body.height;
    float x = 160 - width / 2;
    float y = 240 - height / 2;
    
    int bodyRectangle[] = {
        sprite.Body.x,
        sprite.Body.y,
        sprite.Body.width,
        sprite.Body.height };
    
    int eyesRectangle[] = {
        sprite.Eyes.x,
        sprite.Eyes.y,
        sprite.Eyes.width,
        sprite.Eyes.height };
    
    int backgroundRectangle[] = { 0, 0, 480, 320 };

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    // Draw background:
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Background);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, backgroundRectangle);
    glDrawTexfOES(0, 0, 0, 320, 480);
    
    if (0) {

        // TWO PASSES WITH TWO TEXTURES

        // Enable Blending:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw Noop's body in a yellowish hue:
        glColor4f(1, 0.83f, 0.33f, 1);
        glBindTexture(GL_TEXTURE_2D, m_textures.Body);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);
        glDrawTexfOES(x, y, 0, width, height);

        // Draw Noop's eyes in white:
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, m_textures.Eyes);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);
        glDrawTexfOES(x, y, 0, width, height);

    } else if (0) {
        
        // TWO PASSES WITH ONE TEXTURE

        glEnable(GL_BLEND);
        
        // Draw Noop's body:
        glColor4f(1, 0.83f, 0.33f, 1);
        glBindTexture(GL_TEXTURE_2D, m_textures.Unified);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);
        glDrawTexfOES(x, y, 0, width, height);
        
        // Draw Noop's eyes:
        glColor4f(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, m_textures.Unified);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, eyesRectangle);
        glDrawTexfOES(x, y, 0, width, height);
        
    } else if (0) {
        
        // MULTITEXTURING WITH THE SAME TEXTURE APPLIED TO BOTH STAGES
        
        // This exposes a driver bug.  The driver does not account for
        // unique crop rectangles at each stage.
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(1, 0.83f, 0.33f, 1);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textures.Unified);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);

        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textures.Unified);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, eyesRectangle);
        
        // Render Noop's body and eyes with one draw call:
        glDrawTexfOES(x, y, 0, width, height);
        
        // Disable texturing in both stages:
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        
    } else {
        
        // MULTITEXTURING WITH DIFFERENT TEXTURES APPLIED TO EACH STAGE,
        // BUT WITH THE CROP RECTANGLE BEING THE SAME ACROSS BOTH STAGES.
        
        glColor4f(1, 0.83f, 0.33f, 1);
        glEnable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textures.Body);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);
        
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_textures.Eyes);
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, bodyRectangle);

        // Render Noop's body and eyes with one draw call:
        glDrawTexfOES(x, y, 0, width, height);

        // Disable texturing in both stages:
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);

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
        case OGL_AI_88: {
            GLenum format = GL_LUMINANCE_ALPHA;
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
        default: {
            w = h = 512;
            GLsizei size = max(32, w * h / 2);
            GLenum format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, size, data);
            break;
        }
    }

    return name;
}
    
}