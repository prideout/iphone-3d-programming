#pragma once
#include "Vector.hpp"

struct IApplicationEngine {
    virtual void Initialize(int width, int height) = 0;
    virtual void Render() const = 0;
    virtual void UpdateAnimation(float timeStep) = 0;
    virtual void OnFingerUp(ivec2 location) = 0;
    virtual void OnFingerDown(ivec2 location) = 0;
    virtual void OnFingerMove(ivec2 oldLocation, ivec2 newLocation) = 0;
    virtual ~IApplicationEngine() {}
};

struct IRenderingEngine {
    virtual void Initialize() = 0;
    virtual void Render() const = 0;
    virtual void UpdateAnimation(float timeStep) = 0;
    virtual ~IRenderingEngine() {}
};

IApplicationEngine* CreateApplicationEngine(IRenderingEngine*);

namespace ES1  { IRenderingEngine* CreateRenderingEngine(); }
namespace ES2  { IRenderingEngine* CreateRenderingEngine(); }
