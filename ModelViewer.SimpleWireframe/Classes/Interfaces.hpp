#pragma once
#include "Vector.hpp"
#include "Quaternion.hpp"
#include <vector>
#include <string>

using std::vector;
using std::string;

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

struct Visual {
    vec3 Color;
    ivec2 LowerLeft;
    ivec2 ViewportSize;
    Quaternion Orientation;
};

struct IRenderingEngine {
    virtual void Initialize(const vector<ISurface*>& surfaces) = 0;
    virtual void Render(const vector<Visual>& visuals) const = 0;
    virtual ~IRenderingEngine() {}
};

struct IResourceManager {
    virtual string GetResourcePath() const = 0;
    virtual void LoadPngImage(const string& filename) = 0;
    virtual void* GetImageData() = 0;
    virtual ivec2 GetImageSize() = 0;
    virtual void UnloadImage() = 0;
    virtual ~IResourceManager() {}
};

namespace ParametricViewer { IApplicationEngine* CreateApplicationEngine(IRenderingEngine*); }
namespace ObjViewer    { IApplicationEngine* CreateApplicationEngine(IRenderingEngine*, IResourceManager*); }
namespace Darwin       { IResourceManager* CreateResourceManager(); }
namespace WireframeES1 { IRenderingEngine* CreateRenderingEngine(); }
namespace WireframeES2 { IRenderingEngine* CreateRenderingEngine(); }
namespace SolidES1     { IRenderingEngine* CreateRenderingEngine(); }
namespace SolidES2     { IRenderingEngine* CreateRenderingEngine(); }
namespace SolidGL2     { IRenderingEngine* CreateRenderingEngine(); }
namespace FacetedES1   { IRenderingEngine* CreateRenderingEngine(); }
namespace FacetedES2   { IRenderingEngine* CreateRenderingEngine(); }
namespace DepthViewer  { IRenderingEngine* CreateRenderingEngine(); }
namespace TexturedES1  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
namespace TexturedES2  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
namespace TexturedGL2  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
