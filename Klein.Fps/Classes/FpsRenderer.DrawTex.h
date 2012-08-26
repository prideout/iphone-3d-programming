#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include "../Textures/NumeralsTexture.h"

typedef unsigned int PVRTuint32;

struct PVR_Texture_Header {
    PVRTuint32 dwHeaderSize;
    PVRTuint32 dwHeight;
    PVRTuint32 dwWidth;
    PVRTuint32 dwMipMapCount;
    PVRTuint32 dwpfFlags;
    PVRTuint32 dwTextureDataSize;
    PVRTuint32 dwBitCount;
    PVRTuint32 dwRBitMask;
    PVRTuint32 dwGBitMask;
    PVRTuint32 dwBBitMask;
    PVRTuint32 dwAlphaBitMask;
    PVRTuint32 dwPVR;
    PVRTuint32 dwNumSurfs;
};

class FpsRenderer {
public:
    FpsRenderer(vec2 windowSize)
    {
        m_filterConstant = 0.1;
        m_fps = 0;
        m_previousTime = mach_absolute_time();
        
        glGenTextures(1, &m_textureHandle);
        glBindTexture(GL_TEXTURE_2D, m_textureHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        PVR_Texture_Header* header = (PVR_Texture_Header*) NumeralsTexture;
        const unsigned char* bytes = (unsigned char*) NumeralsTexture;
        const unsigned char* imageData = bytes + header->dwHeaderSize;
        GLenum type = GL_UNSIGNED_BYTE;
        GLenum format = GL_ALPHA;
        int w = header->dwWidth;
        int h = header->dwHeight;
        m_textureSize = vec2(w, h);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, imageData);
    }
    void RenderFps()
    {
        uint64_t deltaTime = GetElapsedNanoseconds();
        double fps = 1000000000.0 / deltaTime;
        double alpha = m_filterConstant;
        m_fps = fps * alpha + m_fps * (1.0 - alpha);
        fps = round(m_fps);
        
        glBindTexture(GL_TEXTURE_2D, m_textureHandle);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glColor4f(1, 1, 1, 1);
        
        char digits[MaxNumDigits + 1] = {0};
        sprintf(digits, "%d", (int) fps);
        vec2 pos(5, 10);

        for (char* digit = &digits[0]; *digit; ++digit) {
            int glyphIndex = *digit - '0';
            const Glyph& glyph = NumeralGlyphs[glyphIndex];
            RenderGlyph(glyph, pos);
            pos.x += glyph.Metrics.XAdvance;
        }
                  
        glDisable(GL_BLEND);
    }
    
private:

    static const int MaxNumDigits = 3;
    
    uint64_t GetElapsedNanoseconds()
    {
        uint64_t current = mach_absolute_time();
        uint64_t duration = current - m_previousTime;
        m_previousTime = current;
        
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        duration *= info.numer;
        duration /= info.denom;
        
        return duration;
    }
    
    void RenderGlyph(const Glyph& glyph, vec2 position)
    { 
        position.y -= glyph.Metrics.Height + glyph.Metrics.YBearing;

        int box[] = { glyph.Position.X,
                      m_textureSize.y - 1 + glyph.Position.Y - glyph.Metrics.Height,
                      glyph.Metrics.Width + 1,
                      glyph.Metrics.Height + 1 };

        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, box);
        glDrawTexfOES(position.x, position.y, 0,
                      glyph.Metrics.Width + 1, glyph.Metrics.Height + 1);
    }

    double m_filterConstant;
    double m_fps;
    uint64_t m_previousTime;
    vec2 m_textureSize;
    GLuint m_textureHandle;
};
