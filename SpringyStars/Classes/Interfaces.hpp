#pragma once
#include "Vector.hpp"
#include <vector>

typedef std::vector<vec2> PositionList;

struct IApplicationEngine {
    virtual void Initialize(int width, int height) = 0;
    virtual void Render() const = 0;
    virtual void UpdateAnimation(float timeStep) = 0;
    virtual void SetGravityDirection(vec2 direction) = 0;
    virtual ~IApplicationEngine() {}
};

struct IRenderingEngine {
    virtual void Initialize() = 0;
    virtual void Render(const PositionList& positions) const = 0;
    virtual ~IRenderingEngine() {}
};

IApplicationEngine* CreateApplicationEngine(IRenderingEngine*);

namespace ES1 { IRenderingEngine* CreateRenderingEngine(); }
namespace ES2 { IRenderingEngine* CreateRenderingEngine(); }
