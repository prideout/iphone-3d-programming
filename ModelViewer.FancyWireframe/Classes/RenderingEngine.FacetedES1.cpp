#if MACOSX
#include <OpenGL/gl.h>
#else
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#endif

#include "Interfaces.hpp"
#include "Matrix.hpp"

namespace FacetedES1 {

struct Drawable {
    GLuint VertexBuffer;
    GLuint TriangleIndexBuffer;
    GLuint LineIndexBuffer;
    int TriangleIndexCount;
    int LineIndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
private:
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    mat4 m_translation;
};
    
IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine()
{
#ifndef MACOSX
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
#endif
}

void RenderingEngine::Initialize(const vector<ISurface*>& surfaces)
{
    vector<ISurface*>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {

        // Create the VBO for the vertices.
        vector<float> vertices;
        (*surface)->GenerateVertices(vertices, VertexFlagsNormals);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(vertices[0]),
                     &vertices[0],
                     GL_STATIC_DRAW);

        // Create a VBO for the triangle indices.
        int triangleIndexCount = (*surface)->GetTriangleIndexCount();
        vector<GLushort> triangleIndices(triangleIndexCount);
        (*surface)->GenerateTriangleIndices(triangleIndices);
        GLuint triangleIndexBuffer;
        glGenBuffers(1, &triangleIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     triangleIndexCount * sizeof(GLushort),
                     &triangleIndices[0],
                     GL_STATIC_DRAW);
        
        
        // Create a VBO for the line indices.
        int lineIndexCount = (*surface)->GetTriangleIndexCount();
        vector<GLushort> lineIndices(lineIndexCount);
        (*surface)->GenerateLineIndices(lineIndices);
        GLuint lineIndexBuffer;
        glGenBuffers(1, &lineIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     lineIndexCount * sizeof(GLushort),
                     &lineIndices[0],
                     GL_STATIC_DRAW);
        
        Drawable drawable = {
            vertexBuffer,
            triangleIndexBuffer,
            lineIndexBuffer,
            triangleIndexCount,
            lineIndexBuffer
        };
        
        m_drawables.push_back(drawable);
    }
#ifndef MACOSX
    // Extract width and height from the color buffer.
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &height);

    // Create a depth buffer that has the same size as the color buffer.
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES,
                             width, height);

    // Create the framebuffer object.
    GLuint framebuffer;
    glGenFramebuffersOES(1, &framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES,
                                 GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
#endif
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Set up the material properties.
    vec4 ambient(0.2f, 0.2f, 0.2f, 1);
    vec4 specular(0.5f, 0.5f, 0.5f, 1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient.Pointer());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular.Pointer());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);

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
        
        // Set the light position.
        glEnable(GL_LIGHTING);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        vec4 lightPosition(0.25, 0.25, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition.Pointer());
        
        // Set the model-view transform.
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelview = rotation * m_translation;
        glLoadMatrixf(modelview.Pointer());
        
        // Set the projection transform.
        float h = 4.0f * size.y / size.x;
        mat4 projection = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.Pointer());
        
        // Set the diffuse color.
        vec3 color = visual->Color * 0.75f;
        vec4 diffuse(color.x, color.y, color.z, 1);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.x);

        // Set up the vertex buffer.
        int stride = 2 * sizeof(vec3);
        const GLvoid* normalOffset = (const GLvoid*) sizeof(vec3);
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexPointer(3, GL_FLOAT, stride, 0);

        // Draw the lit triangles.
        glPolygonOffset(4, 8);
        // glPolygonOffset(1, 1000); // Use this for 1st and 2nd gen devices
        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, normalOffset);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.TriangleIndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.TriangleIndexCount, GL_UNSIGNED_SHORT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        
        // Draw the black lines.
        glColor4f(0, 0, 0, 1);
        glDisable(GL_LIGHTING);
        glDisableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.LineIndexBuffer);
        glDrawElements(GL_LINES, drawable.TriangleIndexCount, GL_UNSIGNED_SHORT, 0);
    }
}
    
}

