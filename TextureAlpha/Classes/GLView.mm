#import "GLView.h"

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

        EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES1;
        m_context = [[EAGLContext alloc] initWithAPI:api];
        
        if (!m_context || ![EAGLContext setCurrentContext:m_context]) {
            [self release];
            return nil;
        }
        
        m_resourceManager = CreateResourceManager();

        NSLog(@"Using OpenGL ES 1.1");
        m_renderingEngine = ES1::CreateRenderingEngine(m_resourceManager);
        
        m_applicationEngine = CreateApplicationEngine(m_renderingEngine);

        [m_context
            renderbufferStorage:GL_RENDERBUFFER
            fromDrawable:eaglLayer];
                
        int width = CGRectGetWidth(frame);
        int height = CGRectGetHeight(frame);
        m_applicationEngine->Initialize(width, height);
        
        [self drawView: nil];
        m_timestamp = CACurrentMediaTime();
        
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
        float elapsedSeconds = displayLink.timestamp - m_timestamp;
        m_timestamp = displayLink.timestamp;
        m_applicationEngine->UpdateAnimation(elapsedSeconds);
    }
    
    m_applicationEngine->Render();
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

@end
