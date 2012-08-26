#include "Interfaces.hpp"
#include "ParametricEquations.hpp"

using namespace std;

namespace ParametricViewer {

static const int SurfaceCount = 6;
static const int ButtonCount = SurfaceCount - 1;

struct Animation {
    bool Active;
    float Elapsed;
    float Duration;
    Visual StartingVisuals[SurfaceCount];
    Visual EndingVisuals[SurfaceCount];
};

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
    void PopulateVisuals(Visual* visuals) const;
    int MapToButton(ivec2 touchpoint) const;
    vec3 MapToSphere(ivec2 touchpoint) const;
    float m_trackballRadius;
    ivec2 m_screenSize;
    ivec2 m_centerPoint;
    ivec2 m_fingerStart;
    bool m_spinning;
    Quaternion m_orientation;
    Quaternion m_previousOrientation;
    int m_currentSurface;
    ivec2 m_buttonSize;
    int m_pressedButton;
    int m_buttonSurfaces[ButtonCount];
    Animation m_animation;
    IRenderingEngine* m_renderingEngine;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_spinning(false),
    m_pressedButton(-1),
    m_renderingEngine(renderingEngine)
{
    m_animation.Active = false;
    m_buttonSurfaces[0] = 0;
    m_buttonSurfaces[1] = 1;
    m_buttonSurfaces[2] = 4;
    m_buttonSurfaces[3] = 3;
    m_buttonSurfaces[4] = 2;
    m_currentSurface = 5;
}

ApplicationEngine::~ApplicationEngine()
{
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height)
{
    m_trackballRadius = width / 3;
    m_buttonSize.y = height / 10;
    m_buttonSize.x = 4 * m_buttonSize.y / 3;
    m_screenSize = ivec2(width, height - m_buttonSize.y);
    m_centerPoint = m_screenSize / 2;

    vector<ISurface*> surfaces(SurfaceCount);
    surfaces[0] = new Cone(3, 1);
    surfaces[1] = new Sphere(1.4f);
    surfaces[2] = new Torus(1.4f, 0.3f);
    surfaces[3] = new TrefoilKnot(1.8f);
    surfaces[4] = new KleinBottle(0.2f);
    surfaces[5] = new MobiusStrip(1);
    m_renderingEngine->Initialize(surfaces);
    for (int i = 0; i < SurfaceCount; i++)
        delete surfaces[i];
}

void ApplicationEngine::PopulateVisuals(Visual* visuals) const
{
    for (int buttonIndex = 0; buttonIndex < ButtonCount; buttonIndex++) {
        
        int visualIndex = m_buttonSurfaces[buttonIndex];
        visuals[visualIndex].Color = vec3(0.25f, 0.25f, 0.25f);
        if (m_pressedButton == buttonIndex)
            visuals[visualIndex].Color = vec3(0.5f, 0.5f, 0.5f);
        
        visuals[visualIndex].ViewportSize = m_buttonSize;
        visuals[visualIndex].LowerLeft.x = buttonIndex * m_buttonSize.x;
        visuals[visualIndex].LowerLeft.y = 0;
        visuals[visualIndex].Orientation = Quaternion();
    }
    
    visuals[m_currentSurface].Color = m_spinning ? vec3(1, 1, 0.75f) : vec3(1, 1, 0.5f);
    visuals[m_currentSurface].LowerLeft = ivec2(0, m_buttonSize.y);
    visuals[m_currentSurface].ViewportSize = ivec2(m_screenSize.x, m_screenSize.y);
    visuals[m_currentSurface].Orientation = m_orientation;
}

void ApplicationEngine::Render() const
{
    vector<Visual> visuals(SurfaceCount);
    
    if (!m_animation.Active) {
        PopulateVisuals(&visuals[0]);
    } else {
        float t = m_animation.Elapsed / m_animation.Duration;
        
        for (int i = 0; i < SurfaceCount; i++) {
            
            const Visual& start = m_animation.StartingVisuals[i];
            const Visual& end = m_animation.EndingVisuals[i];
            Visual& tweened = visuals[i];
            
            tweened.Color = start.Color.Lerp(t, end.Color);
            tweened.LowerLeft = start.LowerLeft.Lerp(t, end.LowerLeft);
            tweened.ViewportSize = start.ViewportSize.Lerp(t, end.ViewportSize);
            tweened.Orientation = start.Orientation.Slerp(t, end.Orientation);
        }
    }
    
    m_renderingEngine->Render(visuals);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    if (m_animation.Active) {
        m_animation.Elapsed += dt;
        if (m_animation.Elapsed > m_animation.Duration)
            m_animation.Active = false;
    }
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_spinning = false;
    
    if (m_pressedButton != -1 && m_pressedButton == MapToButton(location) &&
        !m_animation.Active)
    {
        m_animation.Active = true;
        m_animation.Elapsed = 0;
        m_animation.Duration = 0.25f;
        
        PopulateVisuals(&m_animation.StartingVisuals[0]);
        swap(m_buttonSurfaces[m_pressedButton], m_currentSurface);
        PopulateVisuals(&m_animation.EndingVisuals[0]);
    }
    
    m_pressedButton = -1;
}

void ApplicationEngine::OnFingerDown(ivec2 location)
{
    m_fingerStart = location;
    m_previousOrientation = m_orientation;
    m_pressedButton = MapToButton(location);
    if (m_pressedButton == -1)
        m_spinning = true;
}

void ApplicationEngine::OnFingerMove(ivec2 oldLocation, ivec2 location)
{
    if (m_spinning) {
        vec3 start = MapToSphere(m_fingerStart);
        vec3 end = MapToSphere(location);
        Quaternion delta = Quaternion::CreateFromVectors(start, end);
        m_orientation = delta.Rotated(m_previousOrientation);
    }
    
    if (m_pressedButton != -1 && m_pressedButton != MapToButton(location))
        m_pressedButton = -1;
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

int ApplicationEngine::MapToButton(ivec2 touchpoint) const
{
    if (touchpoint.y  < m_screenSize.y - m_buttonSize.y)
        return -1;
    
    int buttonIndex = touchpoint.x / m_buttonSize.x;
    if (buttonIndex >= ButtonCount)
        return -1;
    
    return buttonIndex;
}

}

