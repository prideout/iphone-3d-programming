#import "GLView.h"
#include "../Textures/TombWindow.h"
#include "../Textures/Circle.h"

#define GL_RENDERBUFFER 0x8d41

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
        
        if (!m_context || ![EAGLContext setCurrentContext:m_context]) {
            NSLog(@"Using OpenGL ES 2.0 is not supported");
            [self release];
            return nil;
        }
        
        NSLog(@"Using OpenGL ES 2.0");
        m_renderingEngine = ES2::CreateRenderingEngine();

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

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint current = [touch locationInView: self];

    vec3 position(0, 0, 0);
    position.x = 3.0f * ((float) current.x - 160.0f) / 160.0f;
    position.y = -5.0f * ((float) current.y - 240.0f) / 240.0f;
    m_renderingEngine->SetSunPosition(position);
}

- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint current = [touch locationInView: self];
    
    vec3 position(0, 0, 0);
    position.x = 3.0f * ((float) current.x - 160.0f) / 160.0f;
    position.y = -5.0f * ((float) current.y - 240.0f) / 240.0f;
    m_renderingEngine->SetSunPosition(position);
}

@end
