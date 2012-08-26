#pragma once
#include "Vector.hpp"
#include <vector>

extern const unsigned int Metal[];
extern const unsigned int Face0[];
extern const unsigned int Face1[];
extern const unsigned int Face2[];
extern const unsigned int Face3[];
extern const unsigned int Face4[];
extern const unsigned int Face5[];

enum VertexFlags {
    VertexFlagsNormals = 1 << 0,
    VertexFlagsTexCoords = 1 << 1,
    VertexFlagsTangents = 1 << 2,
};

typedef std::vector<float> VertexList;
typedef std::vector<unsigned short> IndexList;

struct IRenderingEngine {
    virtual void Initialize() = 0;
    virtual void Render(float theta) const = 0;
    virtual ~IRenderingEngine() {}
};

struct ISurface {
    virtual int GetVertexCount() const = 0;
    virtual int GetTriangleIndexCount() const = 0;
    virtual void GenerateVertices(VertexList& vertices, int flags) const = 0;
    virtual void GenerateTriangleIndices(IndexList& indices) const = 0;
    virtual ~ISurface() {}
};

namespace ES2 { IRenderingEngine* CreateRenderingEngine(); }
