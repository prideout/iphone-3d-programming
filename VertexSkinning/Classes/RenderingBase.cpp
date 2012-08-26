#include "RenderingBase.hpp"
#include "../Textures/Circle.h"
#include "../Textures/Tile.h"
#include "../MotionCapture/StickFigure.h"

const vec3 Eye(0.5f, 1, 62);
const vec3 Target(0.5f, 1, 0);
const vec3 Up(0, 1, 0);

const int JointCount = 18;
const int NumDivisions = 10;
const int BoneCount = sizeof(StickFigureBones) / sizeof(StickFigureBones[0]);
const int FrameCount = 653;
const float LineWidth = 0.03;

extern const float LongMoCap[FrameCount][JointCount][3];

void RenderingBase::Initialize(int maxBoneCount, VertexList& vertices, IndexList& indices)
{
    // Create vertices for full-screen quad:
    m_backgroundVertices.resize(4);
    m_backgroundVertices[0].Position = vec3(-1, -1, 0);
    m_backgroundVertices[0].TexCoord = vec2(0, 0);
    m_backgroundVertices[1].Position = vec3(-1, 1, 0);
    m_backgroundVertices[1].TexCoord = vec2(0, 1);
    m_backgroundVertices[2].Position = vec3(1, -1, 0);
    m_backgroundVertices[2].TexCoord = vec2(1, 0);
    m_backgroundVertices[3].Position = vec3(1, 1, 0);
    m_backgroundVertices[3].TexCoord = vec2(1, 1);
    
    m_skeleton.Vertices.resize(JointCount);
    
    // Create the skeleton:
    m_skeleton.Indices.resize(BoneCount * 2);
    IndexList::iterator index = m_skeleton.Indices.begin();
    for (int boneIndex = 0; boneIndex < BoneCount; boneIndex++) {
        *index++ = StickFigureBones[boneIndex].A;
        *index++ = StickFigureBones[boneIndex].B;
    }
    
    // Create the triangle-based figure:
    GenerateIndices(indices);
    
    size_t vertsPerBone = 8 + NumDivisions * 2;
    vertices.resize(BoneCount * vertsPerBone);
    m_skinnedFigure.Matrices.resize(BoneCount);
    
    GenerateTexCoords(vertices);
    GenerateBoneData(vertices, maxBoneCount);
    
    AnimateSkeleton(0, m_skeleton);
    ComputeMatrices(m_skeleton, m_skinnedFigure.Matrices);
    ExtrudeLines(m_skeleton, vertices, LineWidth);
    
    // Load up some textures:
    m_textures.Tile = CreateTexture(Tile);
    m_textures.Circle = CreateTexture(Circle);
}

void RenderingBase::AnimateSkeleton(float time, Skeleton& skeleton)
{
    time *= 30.0f;
//  time = 98; // For showing off pinching
//  time = 70; // For the screenshot
    int timeIndex = (int) time;
    int frameCount = sizeof(LongMoCap) / sizeof(LongMoCap[0]);
    
    int frameIndex = timeIndex % frameCount;
    float fraction = time - timeIndex;
    
    for (int i = 0; i < JointCount; i++) {
        
        vec3 a;
        a.x = LongMoCap[frameIndex][i][0];
        a.y = LongMoCap[frameIndex][i][1];
        a.z = LongMoCap[frameIndex][i][2];
        
        vec3 b = a;
        if (frameIndex < frameCount - 1) {
            b.x  = LongMoCap[frameIndex + 1][i][0];
            b.y = LongMoCap[frameIndex + 1][i][1];
            b.z = LongMoCap[frameIndex + 1][i][2];
        }
        
        skeleton.Vertices[i].Position = a.Lerp(fraction, b);
    }
}

void RenderingBase::ComputeMatrices(const Skeleton& skeleton, MatrixList& matrices)
{
    mat4 modelview = mat4::LookAt(Eye, Target, Up);
    
    float x = 0;
    IndexList::const_iterator lineIndex = skeleton.Indices.begin();
    for (int boneIndex = 0; boneIndex < BoneCount; ++boneIndex) {
        
        // Compute the length, orientation, and midpoint of this bone:
        float length;
        vec3 orientation, midpoint;
        {
            vec3 a = skeleton.Vertices[*lineIndex++].Position;
            vec3 b = skeleton.Vertices[*lineIndex++].Position;
            length = (b - a).Length();
            orientation = (b - a) / length;
            midpoint = (a + b) * 0.5f;
        }

        // Find the end points of the "unflexed" bone that sits at the origin:
        vec3 a(0, 0, 0);
        vec3 b(length, 0, 0);
        if (StickFigureBones[boneIndex].IsBlended) {
            a.x += x;
            b.x += x;
        }
        x = b.x;
        
        // Compute the matrix that transforms the unflexed bone to its current state:
        vec3 A = orientation;
        vec3 B = vec3(-A.y, A.x, 0);
        vec3 C = A.Cross(B);
        mat3 basis(A, B, C);
        vec3 T = (a + b) * 0.5;
        mat4 rotation = mat4::Translate(-T) * mat4(basis);
        mat4 translation = mat4::Translate(midpoint);
        matrices[boneIndex] = rotation * translation * modelview;
    }
}

void RenderingBase::ExtrudeLines(const Skeleton& skeleton, VertexList& triangles, float width)
{
    float x = 0;
    Vertex* destVertex = &triangles[0];
    IndexList::const_iterator lineIndex = skeleton.Indices.begin();
    for (int boneIndex = 0; boneIndex < BoneCount; ++boneIndex) {

        float length;
        {
            vec3 a = skeleton.Vertices[*lineIndex++].Position;
            vec3 b = skeleton.Vertices[*lineIndex++].Position;
            length = (b - a).Length();
        }
        
        // Unskinned positions:
        vec3 a(0, 0, 0);
        vec3 b(0, 0, 0);
        if (StickFigureBones[boneIndex].IsBlended)
            a.x = x;
        b.x = a.x + length;
        x = b.x;
        
        // Compute the extrusion vectors:
        vec3 E = vec3(width, 0, 0);
        vec3 N = vec3(0, width, 0);
        vec3 S = -N;
        vec3 NE = N + E;
        vec3 NW = N - E;
        vec3 SW = -NE;
        vec3 SE = -NW;
        
        destVertex++->Position = a + SW;
        destVertex++->Position = a + NW;
        destVertex++->Position = a + S;
        destVertex++->Position = a + N;
        
        float dt = 1.0f / (float) (NumDivisions + 1);
        float t = dt;
        for (int i = 0; i < NumDivisions; i++, t += dt) {
            destVertex++->Position = (a + S).Lerp(t, b + S);
            destVertex++->Position = (a + N).Lerp(t, b + N);
        }
        
        destVertex++->Position = b + S;
        destVertex++->Position = b + N;
        destVertex++->Position = b + SE;
        destVertex++->Position = b + NE;
    }
    printf("\n\n");
}

void RenderingBase::GenerateIndices(IndexList& triangles)
{
    size_t vertsPerBone = 8 + NumDivisions * 2;
    size_t destVertCount = BoneCount * vertsPerBone;
    size_t indicesPerBone = 12 + 6 * (NumDivisions + 1);
    triangles.resize(BoneCount * indicesPerBone);
    
    IndexList::iterator triangle = triangles.begin();
    
    for (GLushort v = 0; v < destVertCount; v += vertsPerBone) {
        for (int i = 0, j = 0; i < NumDivisions + 3; i++, j += 2) {
            *triangle++ = j + v; *triangle++ = j + 1 + v; *triangle++ = j + 2 + v;
            *triangle++ = j + 2 + v; *triangle++ = j + 1 + v; *triangle++ = j + 3 + v;
        }
    }
}

void RenderingBase::GenerateTexCoords(VertexList& triangles)
{
    Vertex* destVertex = &triangles[0];
    for (size_t boneIndex = 0; boneIndex < BoneCount; ++boneIndex) {
        destVertex++->TexCoord = vec2(0, 0);
        destVertex++->TexCoord = vec2(0, 1);
        for (int j = 0; j < NumDivisions + 2; ++j) {
            destVertex++->TexCoord = vec2(0.5, 0);
            destVertex++->TexCoord = vec2(0.5, 1);
        }
        destVertex++->TexCoord = vec2(1, 0);
        destVertex++->TexCoord = vec2(1, 1);
    }
}

void RenderingBase::GenerateBoneData(VertexList& triangles, int maxBoneCount)
{
    const int NumSlices = NumDivisions + 4;
    const float Delta = 1.0f / (float) NumSlices;
    Vertex* destVertex = &triangles[0];
    for (size_t boneIndex = 0; boneIndex < BoneCount; ++boneIndex) {

        float blendWeight = boneIndex;
        float delta0 = 0;
        float delta1 = 0;

        if (StickFigureBones[boneIndex].IsBlended)
        {
            delta0 = Delta;
            blendWeight -= 0.5f;
        }
        
        if (boneIndex < BoneCount - 1 && 
            StickFigureBones[boneIndex + 1].IsBlended)
        {
            delta1 = Delta;
        }
        
        for (int j = 0; j < NumSlices; ++j) {
            
            GLushort index0 = floor(blendWeight);
            GLushort index1 = ceil(blendWeight);
            index1 = index1 < BoneCount ? index1 : index0;
            
            int i0 = index0 % maxBoneCount;
            int i1 = index1 % maxBoneCount;
            
            // All passes beyond the first pass are offset by one.
            if (index0 >= maxBoneCount || index1 >= maxBoneCount) {
                i0++;
                i1++;
            }
            
            destVertex->BoneIndices = i1 | (i0 << 8);
            destVertex->BoneWeights.x = blendWeight - index0;
            destVertex->BoneWeights.y = 1.0f - destVertex->BoneWeights.x;
            destVertex++;
            
            destVertex->BoneIndices = i1 | (i0 << 8);
            destVertex->BoneWeights.x = blendWeight - index0;
            destVertex->BoneWeights.y = 1.0f - destVertex->BoneWeights.x;
            destVertex++;

            blendWeight += (j < NumSlices / 2) ? delta0 : delta1;
        }
    }
}

GLuint RenderingBase::CreateTexture(const unsigned long* data)
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
        default:
            printf("Unknown format.\n");
    }

    return name;
}
