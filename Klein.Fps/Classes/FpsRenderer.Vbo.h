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
        m_windowSize = windowSize;
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
        
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        int totalSize = BytesPerDigit * MaxNumDigits;
        glBufferData(GL_ARRAY_BUFFER, totalSize, 0, GL_DYNAMIC_DRAW);
    }
    void RenderFps()
    {
        uint64_t deltaTime = GetElapsedNanoseconds();
        double fps = 1000000000.0 / deltaTime;
        double alpha = m_filterConstant;
        m_fps = fps * alpha + m_fps * (1.0 - alpha);
        fps = round(m_fps);
        
        char digits[MaxNumDigits + 1] = {0};
        sprintf(digits, "%d", (int) fps);
        int numDigits = strlen(digits);
        vec2 pos(5, 10);
        
        vector<float> vbo(numDigits * FloatsPerDigit);
        float* vertex = &vbo[0];
        for (char* digit = &digits[0]; *digit; ++digit) {
            int glyphIndex = *digit - '0';
            const Glyph& glyph = NumeralGlyphs[glyphIndex];
            vertex = WriteGlyphVertex(glyph, pos, 0, vertex);
            vertex = WriteGlyphVertex(glyph, pos, 1, vertex);
            vertex = WriteGlyphVertex(glyph, pos, 2, vertex);
            vertex = WriteGlyphVertex(glyph, pos, 2, vertex);
            vertex = WriteGlyphVertex(glyph, pos, 3, vertex);
            vertex = WriteGlyphVertex(glyph, pos, 1, vertex);
            pos.x += glyph.Metrics.XAdvance;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, BytesPerDigit * numDigits, &vbo[0]);
        glBindTexture(GL_TEXTURE_2D, m_textureHandle);
        glVertexPointer(2, GL_FLOAT, BytesPerVert, 0);
        glTexCoordPointer(2, GL_FLOAT, BytesPerVert, (GLvoid*) TexCoordOffset);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrthof(0, m_windowSize.x, 0, m_windowSize.y, 0, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glColor4f(1, 1, 1, 1);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDrawArrays(GL_TRIANGLES, 0, numDigits * VertsPerDigit);
        glEnableClientState(GL_NORMAL_ARRAY);
        glDisable(GL_BLEND);
    }
    
private:

    static const int MaxNumDigits = 3;
    static const int VertsPerDigit = 6;
    static const int FloatsPerVert = 4;
    static const int FloatsPerDigit = VertsPerDigit * FloatsPerVert;
    static const int TexCoordOffset = sizeof(float) * 2;
    static const int BytesPerVert = sizeof(float) * FloatsPerVert;
    static const int BytesPerDigit = sizeof(float) * FloatsPerDigit;
    
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

    float* WriteGlyphVertex(const Glyph& glyph, vec2 position, int corner, float* vertex)
    {
        vec2 texcoord;
        texcoord.x = glyph.Position.X;
        texcoord.y = glyph.Position.Y + glyph.Metrics.Height;
        
        position.y -= glyph.Metrics.Height + glyph.Metrics.YBearing;
        
        if (corner % 2) {
            position.x += glyph.Metrics.Width;
            texcoord.x += glyph.Metrics.Width;
        }
        
        if (corner / 2) {
            position.y += glyph.Metrics.Height;
            texcoord.y -= glyph.Metrics.Height;
        }
        
        *vertex++ = position.x;
        *vertex++ = position.y;
        *vertex++ = (1 + texcoord.x) / m_textureSize.x;
        *vertex++ = 1 - (1 + texcoord.y) / m_textureSize.y;
        
        return vertex;
    }
    
    double m_filterConstant;
    double m_fps;
    uint64_t m_previousTime;
    vec2 m_windowSize;
    vec2 m_textureSize;
    GLuint m_textureHandle;
    GLuint m_vbo;
};
