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
    float m_theta;
    int m_dragStart;
    int m_dragEnd;
    bool m_dragging;
    IRenderingEngine* m_renderingEngine;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_renderingEngine(renderingEngine),
    m_dragging(false)
{
}

ApplicationEngine::~ApplicationEngine()
{
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height)
{
    m_renderingEngine->Initialize();
}

void ApplicationEngine::Render() const
{
    float theta = m_theta;
    if (m_dragging)
        theta += m_dragEnd - m_dragStart;
    m_renderingEngine->Render(theta);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    if (!m_dragging)
        m_theta += dt * 20;
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_theta += m_dragEnd - m_dragStart;
    m_dragging = false;
}

void ApplicationEngine::OnFingerDown(ivec2 location)
{
    if (!m_dragging) {
        m_dragging = true;  
        m_dragStart = m_dragEnd = location.x;
    }
}

void ApplicationEngine::OnFingerMove(ivec2 oldLocation, ivec2 newLocation)
{
    m_dragEnd = newLocation.x;
}
