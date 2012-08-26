#include "Interfaces.hpp"
#include "ParametricSurface.hpp"

using namespace std;

const string TextureFiles[] = {
    "Grasshopper.png",
    "Utopia4444.pvr",
    "Grasshopper565.pvr",
    "Luma8.png",
    "LumaAlpha8.png",
    "Rgb8.png",
    "Rgba8.png",
    "LetterA.png",
    "Astronomy.pvr", 
    "Utopia.png",
    "Utopia.pvr",
};

class ApplicationEngine : public IApplicationEngine {
public:
    ApplicationEngine(IRenderingEngine* renderingEngine);
    ~ApplicationEngine();
    void Initialize(int width, int height);
    void OnFingerUp(ivec2 location) {}
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation) {}
    void Render() const;
    void UpdateAnimation(float dt);
private:
    void LoadTexture();
    ivec2 m_screenSize;
    ivec2 m_centerPoint;
    IRenderingEngine* m_renderingEngine;
    int m_textureIndex;
    float m_timer;
};
    
IApplicationEngine* CreateApplicationEngine(IRenderingEngine* renderingEngine)
{
    return new ApplicationEngine(renderingEngine);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine* renderingEngine) :
    m_renderingEngine(renderingEngine),
    m_timer(0)
{
}

ApplicationEngine::~ApplicationEngine()
{
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height)
{
    m_screenSize = ivec2(width, height);
    m_centerPoint = m_screenSize / 2;

    vector<ISurface*> surfaces(1);
    surfaces[0] = new Quad(2, 2);
    m_renderingEngine->Initialize(surfaces);
    delete surfaces[0];
    
    m_textureIndex = 0;
    LoadTexture();
}

void ApplicationEngine::Render() const
{
    vector<Visual> visuals(1);
    visuals[0].Color = vec3(1, 1, 1);
    visuals[0].LowerLeft = ivec2(-160, 0);
    visuals[0].ViewportSize = ivec2(m_screenSize.x*2, m_screenSize.y);
    visuals[0].Orientation = Quaternion();
    m_renderingEngine->Render(visuals);
}

void ApplicationEngine::OnFingerDown(ivec2 location)
{
    m_textureIndex++;
    if (m_textureIndex >= sizeof(TextureFiles) / sizeof(TextureFiles[0]))
        m_textureIndex = 0;
    
    LoadTexture();
}

void ApplicationEngine::UpdateAnimation(float dt)
{
    m_timer += dt;
    if (m_timer > 0.75f) {
        m_timer = 0;
        OnFingerDown(ivec2(0, 0));
    }
}

void ApplicationEngine::LoadTexture()
{
    string filename = TextureFiles[m_textureIndex];
    string suffix = ".pvr";
    size_t i = filename.rfind(suffix);
    if (i != string::npos && i == (filename.length() - suffix.length()))
        m_renderingEngine->SetPvrTexture(filename);
    else
        m_renderingEngine->SetPngTexture(filename);
}