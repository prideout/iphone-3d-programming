#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include "../Textures/Star.h"
#include "../Textures/Background.h"

using namespace std;

namespace ES1 {

#include "RenderingEngine.Common.hpp"

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(const PositionList& positions) const;
private:
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
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
    // Load up some textures:
    m_textures.Star = CreateTexture(Star);
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
    glPointSize(15);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // Set up the transforms:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    const float NearPlane = 5, FarPlane = 100;
    const float Scale = 0.0005;
    glFrustumf(-Scale * screenSize.x / 2, Scale * screenSize.x / 2,
               -Scale * screenSize.y / 2, Scale * screenSize.y / 2,
               NearPlane, FarPlane);
    
    glMatrixMode(GL_MODELVIEW);
    
    vec3 eye(0, 0, 40);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);
    mat4 modelview = mat4::LookAt(eye, target, up);
    glLoadMatrixf(modelview.Pointer());
}

void RenderingEngine::Render(const PositionList& positions) const
{
    int backgroundRectangle[] = { 0, 0, 480, 320 };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, backgroundRectangle);
    glBindTexture(GL_TEXTURE_2D, m_textures.Background);
    glColor4f(0.75, 0.75, 0.75, 1);
    glDrawTexfOES(0, 0, 0, 320, 480);

    glBindTexture(GL_TEXTURE_2D, m_textures.Star);
    glEnable(GL_POINT_SPRITE_OES);
    glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(vec2), &positions[0].x);
    glEnable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
    glDrawArrays(GL_POINTS, 0, positions.size());
    
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SPRITE_OES);
    glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_FALSE);
}
    
}