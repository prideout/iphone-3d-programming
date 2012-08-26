#include "Interfaces.hpp"
#include "ParametricSurface.hpp"

using namespace std;

class ApplicationEngine : public IApplicationEngine {
public:
    ApplicationEngine(IRenderingEngine* renderingEngine);
    ~ApplicationEngine();
    void Initialize(int width, int height);
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
    void Render() const;
    void UpdateAnimation(float dt);
private:
    vec3 MapToSphere(ivec2 touchpoint) const;
    float m_trackballRadius;
    ivec2 m_screenSize;
    ivec2 m_centerPoint;
    ivec2 m_fingerStart;
    bool m_spinning;
    bool m_animating;
    Quaternion m_backgroundQuat;
    Quaternion m_orientation;
    Quaternion m_previousOrientation;
    IRenderingEngine* m_renderingEngine;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_spinning(false),
    m_animating(true),
    m_renderingEngine(renderingEngine)
{
    m_orientation = Quaternion::CreateFromVectors(vec3(-1, 0, 0), vec3(0.1, 0, 1));
    m_orientation.Normalize();
}

ApplicationEngine::~ApplicationEngine()
{
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height)
{
    m_trackballRadius = width / 3;
    m_screenSize = ivec2(width, height);
    m_centerPoint = m_screenSize / 2;

    vector<ISurface*> surfaces(2);
    surfaces[0] = new Quad(6, 6);
    surfaces[1] = new TrefoilKnot(2);
    m_renderingEngine->Initialize(surfaces);
    delete surfaces[0];
}

void ApplicationEngine::Render() const
{
    vector<Visual> visuals(2);
    
    visuals[0].Color = vec3(0.5, 0.5, 0.5);
    visuals[0].LowerLeft = ivec2(-160, 0);
    visuals[0].ViewportSize = ivec2(m_screenSize.x * 2, m_screenSize.y);
    visuals[0].Orientation = m_backgroundQuat;

    visuals[1].Color = vec3(1, 1, 1);
    visuals[1].LowerLeft = ivec2(-160, 0);
    visuals[1].ViewportSize = ivec2(m_screenSize.x * 2, m_screenSize.y);
    visuals[1].Orientation = m_orientation;
    
    m_renderingEngine->Render(visuals);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    if (m_animating) {
        vec3 axis(0, 1, 0);
        float angle = M_PI / 2;
        Quaternion spin = Quaternion::CreateFromAxisAngle(axis, angle);
        Quaternion rotated = m_orientation.Rotated(spin);
        m_orientation = m_orientation.Slerp(dt / 5, rotated);
    }
    
    vec3 axis(0, 0, 1);
    static float angle = 0;
    angle += dt * 2;
    m_backgroundQuat = Quaternion::CreateFromAxisAngle(axis, angle);
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_spinning = false;
    m_animating = true;
}

void ApplicationEngine::OnFingerDown(ivec2 location)
{
    m_fingerStart = location;
    m_previousOrientation = m_orientation;
    m_spinning = true;
    m_animating = false;
}

void ApplicationEngine::OnFingerMove(ivec2 oldLocation, ivec2 location)
{
    if (m_spinning) {
        vec3 start = MapToSphere(m_fingerStart);
        vec3 end = MapToSphere(location);
        Quaternion delta = Quaternion::CreateFromVectors(start, end);
        m_orientation = delta.Rotated(m_previousOrientation);
    }
}

vec3 ApplicationEngine::MapToSphere(ivec2 touchpoint) const
{
    vec2 p = touchpoint - m_centerPoint;
    
    // Flip the Y axis because pixel coords increase towards the bottom.
    p.y = -p.y;
    
    const float radius = m_trackballRadius;
    const float safeRadius = radius - 1;
    
    if (p.Length() > safeRadius) {
        float theta = atan2(p.y, p.x);
        p.x = safeRadius * cos(theta);
        p.y = safeRadius * sin(theta);
    }
    
    float z = sqrt(radius * radius - p.LengthSquared());
    vec3 mapped = vec3(p.x, p.y, z);
    return mapped / radius;
}
