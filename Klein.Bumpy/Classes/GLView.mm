#import "GLView.h"
#include "../Textures/Tiger565.h"
#include "../Textures/TangentSpaceNormals.h"
#include "../Textures/ObjectSpaceNormals.h"
#import <OpenGLES/ES2/gl.h> // <-- for GL_RENDERBUFFER only

const bool ForceES1 = false;

@implementation GLView

+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id) initWithFrame: (CGRect) frame
{
    if (self = [super initWithFrame:frame])
    {
        CAEAGLLayer* eaglLayer = (CAEAGLLayer*) self.layer;
        eaglLayer.opaque = YES;

        EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES2;
        m_context = [[EAGLContext alloc] initWithAPI:api];
        
        if (!m_context || ForceES1) {
            api = kEAGLRenderingAPIOpenGLES1;
            m_context = [[EAGLContext alloc] initWithAPI:api];
        }
        
        if (!m_context || ![EAGLContext setCurrentContext:m_context]) {
            [self release];
            return nil;
        }
        
        if (api == kEAGLRenderingAPIOpenGLES1) {
            NSLog(@"Using OpenGL ES 1.1");
            m_renderingEngine = ES1::CreateRenderingEngine();
        } else {
            NSLog(@"Using OpenGL ES 2.0");
            m_renderingEngine = ES2::CreateRenderingEngine();
        }

        [m_context
            renderbufferStorage:GL_RENDERBUFFER
            fromDrawable: eaglLayer];
                
        m_renderingEngine->Initialize();
        
        m_timestamp = -1;
        m_theta = 0;
        [self drawView: nil];
        
        CADisplayLink* displayLink;
        displayLink = [CADisplayLink displayLinkWithTarget:self
                                     selector:@selector(drawView:)];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop]
                     forMode:NSDefaultRunLoopMode];
    }
    return self;
}

- (void) drawView: (CADisplayLink*) displayLink
{
    if (displayLink != nil) {
        if (m_timestamp > 0) {
            float elapsedSeconds = displayLink.timestamp - m_timestamp;
            m_theta += elapsedSeconds;
        }
        m_timestamp = displayLink.timestamp;
    }
    
    m_renderingEngine->Render(m_theta);
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

@end
