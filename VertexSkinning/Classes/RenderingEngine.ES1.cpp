#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "RenderingBase.hpp"

// This file has a certain warning turned off: "-Wno-invalid-offsetof"
// It's safe to use offsetof on non-POD types, as long as they're fairly simple.
#define _offsetof(TYPE, MEMBER) (GLvoid*) (offsetof(TYPE, MEMBER))

using namespace std;

namespace ES1 {

class RenderingEngine : public RenderingBase, public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize();
    void Render() const;
    void UpdateAnimation(float timestamp);
private:
    int m_maxBoneCount;
};

IRenderingEngine* CreateRenderingEngine()
{
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine()
{
    glGenRenderbuffersOES(1, &m_renderbuffers.Screen);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    
    // Retreive the maximum number of matrices for this device:
    glGetIntegerv(GL_MAX_PALETTE_MATRICES_OES, &m_maxBoneCount);
}

void RenderingEngine::Initialize()
{
    IndexList indices;
    VertexList vertices;
    RenderingBase::Initialize(m_maxBoneCount, vertices, indices);

    glGenBuffers(1, &m_skinnedFigure.IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skinnedFigure.IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(indices[0]),
                 &indices[0],
                 GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_skinnedFigure.VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_skinnedFigure.VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(vertices[0]),
                 &vertices[0],
                 GL_STATIC_DRAW);

    // Extract width and height from the color buffer.
    ivec2 screenSize;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_WIDTH_OES, &screenSize.x);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
                                    GL_RENDERBUFFER_HEIGHT_OES, &screenSize.y);
    
    // Create the on-screen FBO.
    glGenFramebuffersOES(1, &m_framebuffers.Screen);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffers.Screen);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
                                 GL_RENDERBUFFER_OES, m_renderbuffers.Screen);
    
    // Set up various GL state.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, screenSize.x, screenSize.y);
    glAlphaFunc(GL_LESS, 0.5);
}
    
void RenderingEngine::UpdateAnimation(float timestamp)
{
    AnimateSkeleton(timestamp, m_skeleton);
    ComputeMatrices(m_skeleton, m_skinnedFigure.Matrices);
}

void RenderingEngine::Render() const
{
    GLsizei stride = sizeof(Vertex);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-1, 1, -1, 1, 0, 100);
    glMatrixMode(GL_MODELVIEW);

    // Set up camera:
    vec3 eye(0, 0, 62);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);
    mat4 modelview = mat4::LookAt(eye, target, up);
    glLoadMatrixf(modelview.Pointer());

    // Draw background:
    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, m_textures.Tile);
    glVertexPointer(3, GL_FLOAT, stride, &m_backgroundVertices[0].Position);
    glTexCoordPointer(2, GL_FLOAT, stride, &m_backgroundVertices[0].TexCoord);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-1, 1, -3, 0, 0, 100);

    // Bind the correct texture:
    glBindTexture(GL_TEXTURE_2D, m_textures.Circle);

    // Render the stick figure:
    glTranslatef(-0.25, -1.25, 0);
    glColor4f(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MATRIX_PALETTE_OES);
    
    const SkinnedFigure& figure = m_skinnedFigure;

    // Set up for skinned rendering:
    glMatrixMode(GL_MATRIX_PALETTE_OES);
    glEnableClientState(GL_WEIGHT_ARRAY_OES);
    glEnableClientState(GL_MATRIX_INDEX_ARRAY_OES);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, figure.IndexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, figure.VertexBuffer);

    glMatrixIndexPointerOES(2, GL_UNSIGNED_BYTE, stride, _offsetof(Vertex, BoneIndices));
    glWeightPointerOES(2, GL_FLOAT, stride, _offsetof(Vertex, BoneWeights));
    glVertexPointer(3, GL_FLOAT, stride, _offsetof(Vertex, Position));
    glTexCoordPointer(2, GL_FLOAT, stride, _offsetof(Vertex, TexCoord));

    // Make several rendering passes if need be, depending on the maximum bone count:
    int startBoneIndex = 0;
    while (startBoneIndex < BoneCount - 1) {
        
        int endBoneIndex = min(BoneCount, startBoneIndex + m_maxBoneCount);

        for (int boneIndex = startBoneIndex; boneIndex < endBoneIndex; ++boneIndex) {

            int slotIndex;
            
            // All passes beyond the first pass are offset by one.
            if (startBoneIndex > 0)
                slotIndex = (boneIndex + 1) % m_maxBoneCount;
            else
                slotIndex = boneIndex % m_maxBoneCount;

            glCurrentPaletteMatrixOES(slotIndex);
            mat4 modelview = figure.Matrices[boneIndex];
            glLoadMatrixf(modelview.Pointer());
        }
        
        size_t indicesPerBone = 12 + 6 * (NumDivisions + 1);
        int startIndex = startBoneIndex * indicesPerBone;
        int boneCount = endBoneIndex - startBoneIndex;
        
        const GLvoid* byteOffset = (const GLvoid*) (startIndex * 2);
        int indexCount = boneCount * indicesPerBone;

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, byteOffset);
        
        startBoneIndex = endBoneIndex - 1;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_MATRIX_PALETTE_OES);
    glDisableClientState(GL_WEIGHT_ARRAY_OES);
    glDisableClientState(GL_MATRIX_INDEX_ARRAY_OES);
}

}
