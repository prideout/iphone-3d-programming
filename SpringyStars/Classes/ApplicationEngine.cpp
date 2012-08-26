#include "Interfaces.hpp"
#include "SpringNode.hpp"

using namespace std;

class ApplicationEngine : public IApplicationEngine {
public:
    ApplicationEngine(IRenderingEngine* renderingEngine);
    ~ApplicationEngine();
    void Initialize(int width, int height);
    void SetGravityDirection(vec2 direction);
    void Render() const;
    void UpdateAnimation(float dt);
private:
    vec2 m_gravityDirection;
    PositionList m_positions;
    vector<SpringNode> m_springNodes;
    IRenderingEngine* m_renderingEngine;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::~ApplicationEngine()
{
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height)
{
    m_renderingEngine->Initialize();
}

void ApplicationEngine::SetGravityDirection(vec2 direction)
{
    m_gravityDirection = direction;
}

void ApplicationEngine::Render() const
{
    m_renderingEngine->Render(m_positions);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_renderingEngine(renderingEngine),
    m_gravityDirection(vec2(0, -1))
{
    const int NumColumns = 10;
    const int NumRows = 14;
    const float SpreadFactor = 0.125f;
    
    m_springNodes.resize(NumColumns * NumRows);
    m_positions.resize(m_springNodes.size());
    
    vector<SpringNode>::iterator node = m_springNodes.begin();
    for (int r = 0; r < NumRows; ++r) {
        for (int c = 0; c < NumColumns; ++c) {
            
            vec2 position;
            position.x = c - (NumColumns - 1) / 2.0f;
            position.y = r - (NumRows - 1) / 2.0f;
            node->SetPosition(position * SpreadFactor);
            
            if (c > 0)
                node->AddNeighbor(&*node - 1);
            
            if (r > 0)
                node->AddNeighbor(&*node - NumColumns);
            
            ++node;
        }
    }
    
    // Pin the four corners so they don't move:
    m_springNodes[0].Pin();
    m_springNodes[NumColumns - 1].Pin();
    m_springNodes[NumColumns * NumRows - 1].Pin();
    m_springNodes[NumColumns * (NumRows - 1)].Pin();
    
    UpdateAnimation(0);
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    const float GravityStrength = 0.01f;
    const int SimulationIterations = 10;
    
    vector<SpringNode>::iterator node;
    vec2 force = m_gravityDirection * GravityStrength;
    
    for (int i = 0; i < SimulationIterations; ++i) {
        for (node = m_springNodes.begin(); node != m_springNodes.end(); ++node)
            node->ResetForce(force);

        for (node = m_springNodes.begin(); node != m_springNodes.end(); ++node)
            node->ComputeForce();

        PositionList::iterator position = m_positions.begin();
        for (node = m_springNodes.begin(); node != m_springNodes.end(); ++node) {
            node->Update(dt);
            *position++ = node->GetPosition();
        }
    }
}
