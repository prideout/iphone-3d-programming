#include "Interfaces.hpp"
#include "ParametricSurface.hpp"

using namespace std;

class ApplicationEngine : public IApplicationEngine {
public:
    ApplicationEngine(IRenderingEngine* renderingEngine);
    ~ApplicationEngine();
    void Initialize(int width, int height);
    void OnFingerUp(ivec2 location) {}
    void OnFingerDown(ivec2 location) {}
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation) {}
    void Render() const;
    void UpdateAnimation(float dt);
private:
    IRenderingEngine* m_renderingEngine;
    float m_theta;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_renderingEngine(renderingEngine)
{
    m_theta = 0;
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
    m_renderingEngine->Render(m_theta);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    m_theta += dt * 20;
}
