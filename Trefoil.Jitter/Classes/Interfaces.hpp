#pragma once
#include "Vector.hpp"
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
    ivec2 OriginalSize;
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

struct IRenderingEngine {
    virtual void Initialize() = 0;
    virtual void Render(float objectTheta, float fboTheta) const = 0;
    virtual ~IRenderingEngine() {}
};

struct IResourceManager {
    virtual TextureDescription LoadImage(const string& filename) = 0;
    virtual void* GetImageData() = 0;
    virtual void UnloadImage() = 0;
    virtual ~IResourceManager() {}
};

IApplicationEngine* CreateApplicationEngine(IRenderingEngine*);
IResourceManager* CreateResourceManager();

namespace ES1  { IRenderingEngine* CreateRenderingEngine(IResourceManager*); }
