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
    float m_timestamp;
    float m_pageCurl;
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
    m_pageCurl(0),
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
    string poem1 = "Haikus are easy\n"
                   "But sometimes they don't make sense\n"
                   "Refrigerator\n";
    
    string poem2 = "Chaos reigns within.\n"
                   "Reflect, repent, and reboot.\n"
                   "Order shall return.";
    
    float pageCurl = m_pageCurl;

    if (m_dragging)
        pageCurl += m_dragStart - m_dragEnd;
    
    if (pageCurl < 0)
        pageCurl = 0;
    pageCurl /= 480.0f;

    m_renderingEngine->Render(poem1, poem2, pageCurl);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    m_timestamp += dt;
    
    // The page falls down when untouched:
    if (!m_dragging && m_pageCurl > 0) {
        m_pageCurl -= dt * 2 * (480 - m_pageCurl);
        if (m_pageCurl < 0)
            m_pageCurl = 0;
    }
}

void ApplicationEngine::OnFingerUp(ivec2 location)
{
    m_pageCurl += m_dragStart - m_dragEnd;
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
