#include "Interfaces.hpp"

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
    float m_timestamp;
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
    m_dragging(false),
    m_timestamp(0)
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
    m_renderingEngine->Render();
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    m_timestamp += dt;
    m_renderingEngine->UpdateAnimation(m_timestamp);
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_dragging = false;
}

void ApplicationEngine::OnFingerDown(ivec2 location)
{
    if (!m_dragging) {
        m_dragging = true;  
        m_dragStart = m_dragEnd = location.y;
    }
}

void ApplicationEngine::OnFingerMove(ivec2 oldLocation, ivec2 newLocation)
{
    m_dragEnd = newLocation.y;
}
