#pragma once
#include <OpenGLES/ES1/gl.h>
#include "Matrix.hpp"
#include <vector>

extern const int JointCount;
extern const int NumDivisions;
extern const int BoneCount;
extern const int FrameCount;
extern const float LineWidth;

struct PVRTextureHeader {
    unsigned int HeaderSize;
    unsigned int Height;
    unsigned int Width;
    unsigned int MipMapCount;
    unsigned int Flags;
};

enum PVRPixelType {
    OGL_RGB_565 = 0x13,
    OGL_I_8 = 0x16,
    PVRTEX_PIXELTYPE = 0xff,
};

struct Framebuffers {
    GLuint Screen;
};

struct Renderbuffers {
    GLuint Screen;
};

struct Textures {
    GLuint Tile;
    GLuint Circle;
};

struct Vertex {
    vec3 Position;
    float Padding0;
    vec2 TexCoord;
    vec2 BoneWeights;
    unsigned short BoneIndices;
    unsigned short Padding1;
};

typedef std::vector<Vertex> VertexList;
typedef std::vector<GLushort> IndexList;
typedef std::vector<mat4> MatrixList;
    
struct Skeleton {
    IndexList Indices;
    VertexList Vertices;
};

struct SkinnedFigure {
    GLuint IndexBuffer;
    GLuint VertexBuffer;
    MatrixList Matrices;
};

class RenderingBase {
protected:
    void Initialize(int maxBoneCount, VertexList& vertices, IndexList& indices);
    GLuint CreateTexture(const unsigned long* data);
    void GenerateBoneData(VertexList& triangles, int maxBoneCount);
    void GenerateTexCoords(VertexList& triangles);
    void GenerateIndices(IndexList& triangles);
    void ExtrudeLines(const Skeleton& skeleton, VertexList& triangles, float width);
    void ComputeMatrices(const Skeleton& skeleton, MatrixList& matrices);
    void AnimateSkeleton(float time, Skeleton& skeleton);
protected:
    Textures m_textures;
    Renderbuffers m_renderbuffers;
    Framebuffers m_framebuffers;
    VertexList m_backgroundVertices;
    Skeleton m_skeleton;
    SkinnedFigure m_skinnedFigure;
};
