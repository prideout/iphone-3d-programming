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
    float m_objectTheta;
    float m_fboTheta; 
    float m_timestamp;
    int m_dragStart;
    int m_dragEnd;
    bool m_dragging;
    float m_fboTransition;
    IRenderingEngine* m_renderingEngine;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_renderingEngine(renderingEngine),
    m_dragging(false),
    m_fboTransition(0),
    m_timestamp(0.5f),
    m_fboTheta(180)
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
    float objectTheta = m_objectTheta;
    if (m_dragging)
        objectTheta += m_dragEnd - m_dragStart;
    
    float fboTheta = m_fboTheta - m_fboTransition;

    // Normalize the angle between 0 and 360.
    int integer = (int) fboTheta;
    float fractional = fboTheta - integer;
    fboTheta = fractional + (integer % 360);
    
    m_renderingEngine->Render(objectTheta, fboTheta);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    // If the user is not dragging the object, spin it.
    if (!m_dragging)
        m_objectTheta += dt * 20;
    
    // If the FBO transition animation is active, update it.
    if (m_fboTransition != 0) {
        m_fboTransition -= dt * 150;
        if (m_fboTransition < 0)
            m_fboTransition = 0;
    }

    // Start a new transition every three seconds.
    m_timestamp += dt;
    if (m_timestamp > 3 && m_fboTransition == 0) {
        m_fboTheta += 180;
        m_fboTransition = 180;
        m_timestamp = 0;
    }
    
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_objectTheta += m_dragEnd - m_dragStart;
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
