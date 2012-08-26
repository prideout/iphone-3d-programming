#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "ParametricSurface.hpp"
#include "Matrix.hpp"

using namespace std;

namespace ES1 {

#include "RenderingEngine.Common.hpp"

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render(float theta) const;
private:
    void RenderDrawable(const Drawable& drawable) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    Drawable m_kleinBottle;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine()
{
    glGenRenderbuffersOES(1, &m_renderbuffers.Color);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Color);
}

void RenderingEngine::Initialize()
{
    // Load up some textures and VBOs:
    m_textures.Tiger = CreateTexture(TigerTexture);
    m_textures.ObjectSpaceNormals = CreateTexture(ObjectSpaceNormals);
    m_kleinBottle = CreateDrawable(KleinBottle(0.2), VertexFlagsTexCoords);
    
    // Extract width and height from the color buffer:
    ivec2 screenSize;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &screenSize.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &screenSize.y);
    
    // Create the depth buffer:
    glGenRenderbuffersOES(1, &m_renderbuffers.Depth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Depth);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES,
                             screenSize.x, screenSize.y);
    
    // Create the on-screen FBO:
    glGenFramebuffersOES(1, &m_framebuffers.Screen);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Color);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Depth);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Color);

    // Set up various GL state:
    glViewport(0, 0, screenSize.x, screenSize.y);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // Set up the transforms:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    const float NearPlane = 5, FarPlane = 50;
    const float Scale = 0.005;
    glFrustumf(-Scale * screenSize.x / 2, Scale * screenSize.x / 2,
               -Scale * screenSize.y / 2, Scale * screenSize.y / 2,
               NearPlane, FarPlane);
    
    glMatrixMode(GL_MODELVIEW);
}

void RenderingEngine::Render(float theta) const
{
    const float distance = 10;
    const vec3 eye(0, 0, distance);
    const vec3 target(0, 0, 0);
    const vec3 up(0, 1, 0);
    const mat4 view = mat4::LookAt(eye, target, up);
    
    const mat4 model = mat4::RotateY(theta * 180.0f / 3.14f);
    const mat4 modelview = model * view;
    glLoadMatrixf(modelview.Pointer());

    glDepthFunc(GL_ALWAYS);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tiger);
    
    int backgroundRectangle[] = { 0, 0, 256, 256 };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, backgroundRectangle);
    glDrawTexfOES(0, 0, 0.75, 320, 480);
    
    glBindTexture(GL_TEXTURE_2D, m_textures.ObjectSpaceNormals);

    vec4 lightWorldSpace = vec4(vec3(0.25, 0.25, 1).Normalized(), 1.0f);
    vec4 lightObjectSpace = model * lightWorldSpace;    
    lightObjectSpace = (lightObjectSpace + vec4(1, 1, 1, 0)) * 0.5f;

    glColor4f(lightObjectSpace.x,
              lightObjectSpace.y,
              lightObjectSpace.z, 1);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);

    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    
    glDepthFunc(GL_LESS);
    RenderDrawable(m_kleinBottle);

    // Restore texture state back to normal:
    glColor4f(1, 1, 1, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void RenderingEngine::RenderDrawable(const Drawable& drawable) const
{
    int stride = sizeof(vec3);
    if (drawable.Flags & VertexFlagsTexCoords)
        stride += sizeof(vec2);
    
    if (drawable.Flags & VertexFlagsNormals)
        stride += sizeof(vec3);
    
    if (drawable.Flags & VertexFlagsTangents)
        stride += sizeof(vec3);
        
    const GLvoid* texCoordOffset = (const GLvoid*) (sizeof(vec3));

    glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
}
    
}