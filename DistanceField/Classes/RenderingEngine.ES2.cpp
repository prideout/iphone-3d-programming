#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <iostream>
#include "Interfaces.hpp"
#include "ParametricSurface.hpp"
#include "Matrix.hpp"
#include "../Textures/DistanceField.h"
#include "../Textures/Tile.h"
#include "../Textures/Text.h"
#include "../Textures/SmallAum.h"
#include "../Textures/SmallText.h"

#define STRINGIFY(A)  #A
#include "../Shaders/SimpleTexturing.es2.vert"
#include "../Shaders/SimpleTexturing.es2.frag"

using namespace std;

namespace ES2 {

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
    DemoStateDistanceFieldSmooth,
    DemoStateDistanceFieldOutline,
    DemoStateDistanceFieldGlow,
    DemoStateDistanceFieldShadow,
    DemoStateCount
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

struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

struct Drawables {
    Drawable Glyph;
    Drawable Square;
    Drawable Label;
    Drawable SmallLabel;
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
    GLuint SmallText;
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
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    Drawables m_drawables;
    AttributeHandles m_attributes;
    UniformHandles m_uniforms;
    ivec2 m_screenSize;
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

bool SupportsDerivatives()
{
    const char* extensions = (const char*) glGetString(GL_EXTENSIONS);
    return 0 != strstr(extensions, "OES_standard_derivatives");
}
    
void RenderingEngine::Initialize()
{
    // Create vertex buffer objects.
    m_drawables.Glyph = CreateDrawable(Quad(2, 1));
    m_drawables.Square = CreateDrawable(Quad(3, 3));
    m_drawables.Label = CreateDrawable(Quad(0.75f, 0.125f));
    m_drawables.SmallLabel = CreateDrawable(Quad(0.5f, 0.0625f));
    
    // Load up some textures.
    m_textures.DistanceField = CreateTexture(DistanceField);
    m_textures.SmallAum = CreateTexture(SmallAum);
    m_textures.Tile = CreateTexture(Tile);
    m_textures.Text = CreateTexture(Text);
    m_textures.SmallText = CreateTexture(SmallText);

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
    const char* fragShader = SupportsDerivatives() ? DeviceFragmentShader :
                                                     SimulatorFragmentShader;
    GLuint program = BuildProgram(SimpleVertexShader, fragShader);
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
    
void RenderingEngine::Render(float timestamp) const
{
    if (false) {
        int desiredState = DemoStateDistanceFieldSmooth;
        float desiredSegment = 0 + desiredState * NumCameraPositions;
        timestamp = desiredSegment * NumCameraPositions / CameraSpeed;
    }
    
    float glowPulse = 0.015 * sin(30 * timestamp);
    float segment = timestamp * CameraSpeed / NumCameraPositions;
    int integer = (int) segment;
    float fract = segment - integer;
    vec3 from = CameraPositions[integer % NumCameraPositions];
    vec3 to = CameraPositions[(integer + 1) % NumCameraPositions];
    
    vec3 eye = from.Lerp(fract, to);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);

    mat4 textureMatrix;
    glUniformMatrix4fv(m_uniforms.TextureMatrix, 1, 0, textureMatrix.Pointer());

    mat4 modelview = mat4::LookAt(eye, target, up);
    glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());

    glUniform1f(m_uniforms.AlphaTest, 2);
    glUniform3f(m_uniforms.GlyphColor, 0, 0, 0);
    glUniform1i(m_uniforms.Smooth, GL_FALSE);
    glUniform1i(m_uniforms.Outline, GL_FALSE);
    glUniform1i(m_uniforms.Glow, GL_FALSE);
    glUniform1i(m_uniforms.Shadow, GL_FALSE);
    
    // Render the background quad:
    glBindTexture(GL_TEXTURE_2D, m_textures.Tile);
    RenderDrawable(m_drawables.Square);

    // Render the Aum symbol:
    DemoState state = (DemoState) ((integer / NumCameraPositions) % DemoStateCount);
    switch (state) {
        case DemoStateAlphaBlended:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glBindTexture(GL_TEXTURE_2D, m_textures.SmallAum);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateAlphaTested:
            glUniform1f(m_uniforms.AlphaTest, 0.5f);
            glBindTexture(GL_TEXTURE_2D, m_textures.SmallAum);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateDistanceField:
            glUniform1f(m_uniforms.AlphaTest, 0.5f);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateDistanceFieldSmooth:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glUniform1i(m_uniforms.Smooth, GL_TRUE);
            glUniform2f(m_uniforms.SmoothRange, 0.46, 0.54);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateDistanceFieldOutline:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glUniform3f(m_uniforms.GlyphColor, 1, 1, 1);
            glUniform2f(m_uniforms.SmoothRange, 0.46, 0.54);
            glUniform1i(m_uniforms.Outline, GL_TRUE);
            glUniform2f(m_uniforms.OutlineRange, 0.42, 0.46);
            glUniform3f(m_uniforms.OutlineColor, 0, 0, 0);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateDistanceFieldGlow:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glUniform1i(m_uniforms.Glow, GL_TRUE);
            glUniform2f(m_uniforms.GlowRange, 0.46 + glowPulse, 0.5 + glowPulse);
            glUniform2f(m_uniforms.GlowOffset, 0, 0);
            glUniform3f(m_uniforms.GlowColor, 1, 0, 0);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
            break;
        case DemoStateDistanceFieldShadow:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            glUniform3f(m_uniforms.GlyphColor, 1, 1, 1);
            glUniform1i(m_uniforms.Shadow, GL_TRUE);
            glUniform2f(m_uniforms.GlowRange, 0.5, 0.51);
            glUniform2f(m_uniforms.GlowOffset, 0.005, -0.005f * 2.0f);
            glUniform3f(m_uniforms.GlowColor, 0.25, 0.25, 0.275);
            glUniform2f(m_uniforms.SmoothRange, 0.6, 0.75);
            glBindTexture(GL_TEXTURE_2D, m_textures.DistanceField);
            RenderDrawable(m_drawables.Glyph);
    }

    glUniform1f(m_uniforms.AlphaTest, 2);
    glUniform3f(m_uniforms.GlyphColor, 1, 1, 1);
    glUniform1i(m_uniforms.Smooth, GL_FALSE);
    glUniform1f(m_uniforms.Outline, GL_FALSE);
    glUniform1i(m_uniforms.Glow, GL_FALSE);
    glUniform1i(m_uniforms.Shadow, GL_FALSE);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render the top label:
    glUniform3f(m_uniforms.GlyphColor, 0.2f, 0.3f, 0.6f);
    glBindTexture(GL_TEXTURE_2D, m_textures.Text);
    modelview = mat4::LookAt(vec3(0, -0.5f, 25), vec3(0, -0.5f, 0), up);
    glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
    textureMatrix = mat4::Scale(1, 0.33, 1);
    switch (state) {
        case DemoStateAlphaBlended:
            textureMatrix = mat4::Translate(0, -1, 0) * textureMatrix;
            break;
        case DemoStateAlphaTested:
            textureMatrix = mat4::Translate(0, 1, 0) * textureMatrix;
            break;
    }
    glUniformMatrix4fv(m_uniforms.TextureMatrix, 1, 0, textureMatrix.Pointer());
    RenderDrawable(m_drawables.Label);

    // Render the bottom label:
    if (state > DemoStateDistanceField) {
        glBindTexture(GL_TEXTURE_2D, m_textures.SmallText);
        modelview = mat4::LookAt(vec3(0, 0.5f, 25), vec3(0, 0.5f, 0), up);
        glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
        textureMatrix = mat4::Scale(1, 0.25, 1);
        switch (state) {
            case DemoStateDistanceFieldSmooth:
                textureMatrix = mat4::Translate(0, -1, 0) * textureMatrix;
                break;
            case DemoStateDistanceFieldOutline:
                textureMatrix = mat4::Translate(0, 2, 0) * textureMatrix;
                break;
            case DemoStateDistanceFieldGlow:
                textureMatrix = mat4::Translate(0, 1, 0) * textureMatrix;
                break;
            case DemoStateDistanceFieldShadow:
                textureMatrix = mat4::Translate(0, 0, 0) * textureMatrix;
                break;
        }
        glUniformMatrix4fv(m_uniforms.TextureMatrix, 1, 0, textureMatrix.Pointer());
        RenderDrawable(m_drawables.SmallLabel);
    }

    glDisable(GL_BLEND);
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
    
    GLint position = m_attributes.Position;
    GLint texCoord = m_attributes.TextureCoord;
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, stride, texCoordOffset);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
    glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
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