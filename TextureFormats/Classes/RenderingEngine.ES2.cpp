#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace ES2 {

#define STRINGIFY(A)  #A
#include "../Shaders/TexturedLighting.vert"
#include "../Shaders/TexturedLighting.frag"

struct UniformHandles {
    GLuint Modelview;
    GLuint Projection;
    GLuint NormalMatrix;
    GLuint LightPosition;
    GLint AmbientMaterial;
    GLint SpecularMaterial;
    GLint Shininess;
    GLint Sampler;
};

struct AttributeHandles {
    GLint Position;
    GLint Normal;
    GLint DiffuseMaterial;
    GLint TextureCoord;
};
    
struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine(IResourceManager*);
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
    void SetPngTexture(const string& name) const;
    void SetPvrTexture(const string& file) const;
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    mat4 m_translation;
    UniformHandles m_uniforms;
    AttributeHandles m_attributes;
	IResourceManager* m_resourceManager;
};

IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
{
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager* resourceManager)
{
	m_resourceManager = resourceManager;
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
}

void RenderingEngine::Initialize(const vector<ISurface*>& surfaces)
{
    vector<ISurface*>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
        
        // Create the VBO for the vertices.
        vector<float> vertices;
		unsigned char vertexFlags = VertexFlagsTexCoords;
        (*surface)->GenerateVertices(vertices, vertexFlags);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(vertices[0]),
                     &vertices[0],
                     GL_STATIC_DRAW);
        
        // Create a new VBO for the indices if needed.
        int indexCount = (*surface)->GetTriangleIndexCount();
        GLuint indexBuffer;
        if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
            indexBuffer = m_drawables[0].IndexBuffer;
        } else {
            vector<GLushort> indices(indexCount);
            (*surface)->GenerateTriangleIndices(indices);
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         indexCount * sizeof(GLushort),
                         &indices[0],
                         GL_STATIC_DRAW);
        }
        
        Drawable drawable = { vertexBuffer, indexBuffer, indexCount};
        m_drawables.push_back(drawable);
    }
    
    // Extract width and height.
    int width, height;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                 GL_RENDERBUFFER_HEIGHT, &height);
    
    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    
    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, m_colorRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    
    // Create the GLSL program.
    GLuint program = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(program);

    // Extract the handles to attributes and uniforms.
    m_attributes.Position = glGetAttribLocation(program, "Position");
    m_attributes.Normal = glGetAttribLocation(program, "Normal");
    m_attributes.DiffuseMaterial = glGetAttribLocation(program, "DiffuseMaterial");
    m_attributes.TextureCoord = glGetAttribLocation(program, "TextureCoord");
    m_uniforms.Projection = glGetUniformLocation(program, "Projection");
    m_uniforms.Modelview = glGetUniformLocation(program, "Modelview");
    m_uniforms.NormalMatrix = glGetUniformLocation(program, "NormalMatrix");
    m_uniforms.LightPosition = glGetUniformLocation(program, "LightPosition");
    m_uniforms.AmbientMaterial = glGetUniformLocation(program, "AmbientMaterial");
    m_uniforms.SpecularMaterial = glGetUniformLocation(program, "SpecularMaterial");
    m_uniforms.Shininess = glGetUniformLocation(program, "Shininess"); 
    m_uniforms.Sampler = glGetUniformLocation(program, "Sampler");
    
	// Set the active sampler to stage 0.  Not really necessary since the uniform
    // defaults to zero anyway, but good practice.
	glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_uniforms.Sampler, 0);
	
    // Set up the texture state.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Set up some default material parameters.
    glUniform3f(m_uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
    glUniform3f(m_uniforms.SpecularMaterial, 0.5, 0.5, 0.5);
    glUniform1f(m_uniforms.Shininess, 50);

    // Initialize various state.
    glEnableVertexAttribArray(m_attributes.Position);
    glEnableVertexAttribArray(m_attributes.TextureCoord);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up transforms.
    m_translation = mat4::Translate(0, 0, -7);
}

void RenderingEngine::Render(const vector<Visual>& visuals) const
{
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {

        // Set the viewport transform.
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
        
        // Set the model-view transform.
        mat4 modelview = m_translation;
        glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
        
        // Set the projection transform.
        float h = 4.0f * size.y / size.x;
        mat4 projectionMatrix = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
        glUniformMatrix4fv(m_uniforms.Projection, 1, 0, projectionMatrix.Pointer());
        
        // Set the diffuse color.
        vec3 color = visual->Color * 0.75f;
        glVertexAttrib4f(m_attributes.DiffuseMaterial, color.x, color.y, color.z, 1);
        
        // Draw the surface.
        int stride = sizeof(vec3) + sizeof(vec2);
        const GLvoid* texCoordOffset = (const GLvoid*) sizeof(vec3);
        GLint position = m_attributes.Position;
        GLint texCoord = m_attributes.TextureCoord;
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, 0);
        glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, stride, texCoordOffset);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
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
        cout << messages;
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
        cout << messages;
        exit(1);
    }
    
    return programHandle;
}

void RenderingEngine::SetPngTexture(const string& filename) const
{
    TextureDescription description = m_resourceManager->LoadImage(filename);
    //TextureDescription description = m_resourceManager->GenerateCircle();
    
    GLenum format;
    switch (description.Format) {
        case TextureFormatGray:      format = GL_LUMINANCE;       break;
        case TextureFormatGrayAlpha: format = GL_LUMINANCE_ALPHA; break;
        case TextureFormatRgb:       format = GL_RGB;             break;
        case TextureFormatRgba:      format = GL_RGBA;            break;
    }

    GLenum type;
    switch (description.BitsPerComponent) {
        case 8: type = GL_UNSIGNED_BYTE; break;
        case 4:
            if (format == GL_RGBA) {
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            }
            // intentionally fall through
        default:
            assert(!"Unsupported format.");
    }

    void* data = m_resourceManager->GetImageData();
    ivec2 size = description.Size;
    glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, type, data);
    m_resourceManager->UnloadImage();
}

void RenderingEngine::SetPvrTexture(const string& filename) const
{
    TextureDescription description = m_resourceManager->LoadPvrImage(filename);
    unsigned char* data = (unsigned char*) m_resourceManager->GetImageData();
    int width = description.Size.x;
    int height = description.Size.y;
    
    int bitsPerPixel;
    GLenum format;
    bool compressed = true;
    switch (description.Format) {
        case TextureFormatPvrtcRgba2:
            bitsPerPixel = 2;
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgb2:
            bitsPerPixel = 2;
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgba4:
            bitsPerPixel = 4;
            format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            break;
        case TextureFormatPvrtcRgb4:
            bitsPerPixel = 4;
            format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            break;
        default:
            compressed = false;
            break;
    }
    
    if (compressed) {
        for (int level = 0; level < description.MipCount; ++level) {
            GLsizei size = max(32, width * height * bitsPerPixel / 8);
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, size, data);
            data += size;
            width >>= 1; height >>= 1;
        }
    } else {
        GLenum type;
        switch (description.Format) {
            case TextureFormatRgba:
                assert(description.BitsPerComponent == 4);
                format = GL_RGBA;
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                bitsPerPixel = 16;
                break;
            case TextureFormat565:
                format = GL_RGB;
                type = GL_UNSIGNED_SHORT_5_6_5;
                bitsPerPixel = 16;
                break;
            case TextureFormat5551:
                format = GL_RGBA;
                type = GL_UNSIGNED_SHORT_5_5_5_1;
                bitsPerPixel = 16;
                break;
        }
        for (int level = 0; level < description.MipCount; ++level) {
            GLsizei size = width * height * bitsPerPixel / 8;
            glTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, format, type, data);
            data += size;
            width >>= 1; height >>= 1;
        }
    }
    
    m_resourceManager->UnloadImage();
}

}
