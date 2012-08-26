#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "Box.hpp"
#include "Matrix.hpp"
#include "../Textures/Unified.h"
#include "../Textures/BodyLayer.h"
#include "../Textures/EyesLayer.h"
#include "../Textures/Tile.h"
#include "../Textures/Background.h"
#include <vector>
#include <iostream>

#define STRINGIFY(A)  #A
#include "../Shaders/SimpleTexturing.es2.vert"
#include "../Shaders/SimpleTexturing.es2.frag"

using namespace std;

namespace ES2 {

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

struct UniformHandles {
    GLuint Modelview;
    GLuint Projection;
    GLuint TextureMatrix;
    GLuint Sampler;
    GLuint AlphaTest;
    GLuint OutlineRange;
    GLuint OutlineColor;
    GLuint Outline;
    GLuint Smooth;
    GLuint SmoothRange;
    GLuint Glow;
    GLuint GlowRange;
    GLuint GlowColor;
    GLuint GlowOffset;
    GLuint GlyphColor;
    GLuint Shadow;
};

struct AttributeHandles {
    GLint Position;
    GLint TextureCoord;
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
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    AttributeHandles m_attributes;
    UniformHandles m_uniforms;
    ivec2 m_screenSize;
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
    glGenRenderbuffers(1, &m_renderbuffers.Screen);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers.Screen);
}

void PrettyPrintExtensions()
{
    string extensions = (const char*) glGetString(GL_EXTENSIONS);
    char* extensionStart = &extensions[0];
    char** extension = &extensionStart;
    cout << "Supported OpenGL ES Extensions:" << endl;
    while (*extension)
        cout << '\t' << strsep(extension, " ") << endl;
    cout << endl;
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
    
    // Extract width and height from the color buffer.
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_WIDTH, &m_screenSize.x);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_HEIGHT, &m_screenSize.y);
    
    // Create the on-screen FBO.
    glGenFramebuffers(1, &m_framebuffers.Screen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers.Screen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_renderbuffers.Screen);
    
    // Create the GLSL program.
    GLuint program = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(program);
    
    // Extract the handles to attributes and uniforms.
    m_attributes.Position = glGetAttribLocation(program, "Position");
    m_attributes.TextureCoord = glGetAttribLocation(program, "TextureCoord");
    m_uniforms.Projection = glGetUniformLocation(program, "Projection");
    m_uniforms.Modelview = glGetUniformLocation(program, "Modelview");
    m_uniforms.TextureMatrix = glGetUniformLocation(program, "TextureMatrix");
    m_uniforms.Sampler = glGetUniformLocation(program, "Sampler");
    m_uniforms.AlphaTest = glGetUniformLocation(program, "AlphaTest");
    m_uniforms.OutlineRange = glGetUniformLocation(program, "OutlineRange");
    m_uniforms.OutlineColor = glGetUniformLocation(program, "OutlineColor");
    m_uniforms.Outline = glGetUniformLocation(program, "Outline");
    m_uniforms.Smooth = glGetUniformLocation(program, "Smooth");
    m_uniforms.SmoothRange = glGetUniformLocation(program, "SmoothRange");
    m_uniforms.Glow = glGetUniformLocation(program, "Glow");
    m_uniforms.GlowRange = glGetUniformLocation(program, "GlowRange");
    m_uniforms.GlowColor = glGetUniformLocation(program, "GlowColor");
    m_uniforms.GlowOffset = glGetUniformLocation(program, "GlowOffset");
    m_uniforms.Shadow = glGetUniformLocation(program, "Shadow");
    m_uniforms.GlyphColor = glGetUniformLocation(program, "GlyphColor");
    
	// Set the active sampler to stage 0.  Not really necessary since the uniform
    // defaults to zero anyway, but good practice.
	glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_uniforms.Sampler, 0);
	
    // Set up various GL state.
    glEnableVertexAttribArray(m_attributes.Position);
    glEnableVertexAttribArray(m_attributes.TextureCoord);
    glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, m_screenSize.x, m_screenSize.y);
    
    // Set up the transforms.
    const float NearPlane = 5, FarPlane = 100;
    const float Scale = 0.0005;
    const float HalfWidth = Scale * m_screenSize.x / 2;
    const float HalfHeight = Scale * m_screenSize.y / 2;
    
    mat4 projection = mat4::Frustum(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight,
                                    NearPlane, FarPlane);
    glUniformMatrix4fv(m_uniforms.Projection, 1, 0, projection.Pointer());
}
    
void RenderingEngine::Render() const
{
    vec3 eye(0, 0, 20);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);

    mat4 textureMatrix;
    glUniformMatrix4fv(m_uniforms.TextureMatrix, 1, 0, textureMatrix.Pointer());

    mat4 modelview = mat4::LookAt(eye, target, up);
    glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
}
    
void RenderingEngine::UpdateAnimation(float timestamp)
{
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
            std::cout << "Unknown format.\n";
            break;
    }

    return name;
}

GLuint RenderingEngine::BuildShader(const char* source, GLenum shaderType) const
{
    GLuint shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
    
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    
    return shaderHandle;
}

GLuint RenderingEngine::BuildProgram(const char* vertexShaderSource,
                                     const char* fragmentShaderSource) const
{
    GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram(programHandle);
    
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
        std::cout << messages;
        exit(1);
    }
    
    return programHandle;
}

}