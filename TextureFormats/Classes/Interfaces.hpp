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

enum TextureFormat {
    TextureFormatGray,
    TextureFormatGrayAlpha,
    TextureFormatRgb,
    TextureFormatRgba,
    TextureFormatPvrtcRgb2,
    TextureFormatPvrtcRgba2,
    TextureFormatPvrtcRgb4,
    TextureFormatPvrtcRgba4,
    TextureFormat565,
    TextureFormat5551,
};

struct TextureDescription {
    TextureFormat Format;
    int BitsPerComponent;
    ivec2 Size;
    int MipCount;
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
    virtual void SetPngTexture(const string& file) const = 0;
    virtual void SetPvrTexture(const string& file) const = 0;
    virtual ~IRenderingEngine() {}
};

struct IResourceManager {
    virtual string GetResourcePath() const = 0;
    virtual TextureDescription LoadPngImage(const string& filename) = 0;
    virtual TextureDescription LoadPvrImage(const string& filename) = 0;
    virtual TextureDescription LoadImage(const string& filename) = 0;
    virtual TextureDescription GenerateCircle() = 0;
    virtual void* GetImageData() = 0;
    virtual void UnloadImage() = 0;
    virtual ~IResourceManager() {}
};

IApplicationEngine* CreateApplicationEngine(IRenderingEngine*);
IResourceManager* CreateResourceManager();

namespace ES1  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
namespace ES2  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
