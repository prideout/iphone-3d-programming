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
    TextureFormatPvrRgb,
    TextureFormatPvrRgba,
};

struct TextureDescription {
    TextureFormat Format;
    int BitsPerComponent;
    ivec2 Size;
    ivec2 OriginalSize;
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
    virtual void Render(float zScale, float theta, bool waiting) const = 0;
    virtual void LoadCameraTexture(const TextureDescription&, void* data) = 0;
    virtual ~IRenderingEngine() {}
};

struct IResourceManager {
    virtual TextureDescription LoadImagePot(const string& image) = 0;
    virtual void* GetImageData() = 0;
    virtual void UnloadImage() = 0;
    virtual ~IResourceManager() {}
};

IRenderingEngine* CreateRenderingEngine(IResourceManager*);
IResourceManager* CreateResourceManager();
