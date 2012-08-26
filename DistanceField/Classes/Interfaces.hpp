#pragma once
#include "Vector.hpp"
#include <vector>

using std::vector;

enum VertexFlags {
    VertexFlagsNormals = 1 << 0,
    VertexFlagsTexCoords = 1 << 1,
};

struct IApplicationEngine {
    virtual void Initialize(int width, int height) = 0;
    virtual void Render() const = 0;
    virtual void UpdateAnimation(float timeStep) = 0;
    virtual void OnFingerUp(ivec2 location) = 0;
    virtual void OnFingerDown(ivec2 location) = 0;
    virtual void OnFingerMove(ivec2 oldLocation, ivec2 newLocation) = 0;
    virtual ~IApplicationEngine() {}
};

struct ISurface {
    virtual int GetVertexCount() const = 0;
    virtual int GetLineIndexCount() const = 0;
    virtual int GetTriangleIndexCount() const = 0;
    virtual void GenerateVertices(vector<float>& vertices,
                                  unsigned char flags = 0) const = 0;
    virtual void GenerateLineIndices(vector<unsigned short>& indices) const = 0;
    virtual void GenerateTriangleIndices(vector<unsigned short>& indices) const = 0;
    virtual ~ISurface() {}
};

struct IRenderingEngine {
    virtual void Initialize() = 0;
    virtual void Render(float timestamp) const = 0;
    virtual ~IRenderingEngine() {}
};

IApplicationEngine* CreateApplicationEngine(IRenderingEngine*);

namespace ES1  { IRenderingEngine* CreateRenderingEngine(); }
namespace ES2  { IRenderingEngine* CreateRenderingEngine(); }
