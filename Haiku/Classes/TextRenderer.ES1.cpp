#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "../Textures/GlyphsTexture.h"
#include "Interfaces.hpp"

typedef unsigned int PVRTuint32;

struct PVR_Texture_Header
{
	PVRTuint32 dwHeaderSize;			/*!< size of the structure */
	PVRTuint32 dwHeight;				/*!< height of surface to be created */
	PVRTuint32 dwWidth;				/*!< width of input surface */
	PVRTuint32 dwMipMapCount;			/*!< number of mip-map levels requested */
	PVRTuint32 dwpfFlags;				/*!< pixel format flags */
	PVRTuint32 dwTextureDataSize;		/*!< Total size in bytes */
	PVRTuint32 dwBitCount;			/*!< number of bits per pixel  */
	PVRTuint32 dwRBitMask;			/*!< mask for red bit */
	PVRTuint32 dwGBitMask;			/*!< mask for green bits */
	PVRTuint32 dwBBitMask;			/*!< mask for blue bits */
	PVRTuint32 dwAlphaBitMask;		/*!< mask for alpha channel */
	PVRTuint32 dwPVR;					/*!< magic number identifying pvr file */
	PVRTuint32 dwNumSurfs;			/*!< the number of surfaces present in the pvr */
};

namespace ES1 {

class TextRenderer : public ITextRenderer {
public:
    void Initialize();
    void RenderText(ivec2 position, const string& text) const;
private:
    void RenderGlyph(const Glyph& glyph, vec2 position) const;
    vec2 m_textureSize;
    GLuint m_textureHandle;
    static const int LineHeight = 25;
};
    
ITextRenderer* CreateTextRenderer()
{
    return new TextRenderer();
}
    
void TextRenderer::Initialize()
{
    glGenTextures(1, &m_textureHandle);
    glBindTexture(GL_TEXTURE_2D, m_textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    PVR_Texture_Header* header = (PVR_Texture_Header*) GlyphsTexture;
    const unsigned char* bytes = (unsigned char*) GlyphsTexture;
    const unsigned char* imageData = bytes + header->dwHeaderSize;
    GLenum type = GL_UNSIGNED_BYTE;
    GLenum format = GL_ALPHA;
    int w = header->dwWidth;
    int h = header->dwHeight;
    m_textureSize = vec2(w, h);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, type, imageData);
}
    
void TextRenderer::RenderText(ivec2 position, const string& text) const
{
    glBindTexture(GL_TEXTURE_2D, m_textureHandle);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    
    int x = position.x;
    for (const char* digit = &text[0]; *digit; ++digit) {
        if (*digit == '\n') {
            position.x = x;
            position.y -= LineHeight;
            continue;
        }
        int glyphIndex = *digit - '!' + 3;
        const Glyph& glyph = Glyphs[glyphIndex];
        RenderGlyph(glyph, position);
        position.x += glyph.Metrics.XAdvance;
    }
              
    glDisable(GL_BLEND);
}
    
void TextRenderer::RenderGlyph(const Glyph& glyph, vec2 position) const
{
    position.x += glyph.Metrics.XBearing;
    position.y -= glyph.Metrics.Height + glyph.Metrics.YBearing;

    int box[] = { glyph.Position.X,
                  m_textureSize.y - 1 - glyph.Position.Y - glyph.Metrics.Height,
                  glyph.Metrics.Width + 1,
                  glyph.Metrics.Height + 1 };
    
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, box);
    glDrawTexfOES(position.x, position.y, 0,
                  glyph.Metrics.Width + 1, glyph.Metrics.Height + 1);
}

}
