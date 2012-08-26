#import "GLView.h"
#import "../Textures/TombWindow.h"
#import <OpenGLES/ES2/gl.h> // <-- for GL_RENDERBUFFER only
#import <string>

void ReadShaderFile(const std::string& file, std::string& contents)
{
    NSString* basePath = [NSString stringWithUTF8String:file.c_str()];
    NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
    NSString* shadersPath = [resourcePath stringByAppendingPathComponent:@"Shaders"];
    NSString* fullPath = [shadersPath stringByAppendingPathComponent:basePath];
    NSData* data = [NSData dataWithContentsOfFile:fullPath];
    contents = std::string((const char*) [data bytes], data.length);
}

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
        m_dragging = false;
        
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
            if (!m_dragging)
                m_theta += elapsedSeconds;
        }
        m_timestamp = displayLink.timestamp;
    }
    
    float theta = m_theta;
    if (m_dragging)
        theta -= (m_dragEnd - m_dragStart) * 0.01f;

    m_renderingEngine->Render(theta);
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    if (!m_dragging) {
        m_dragging = true;  
        m_dragStart = m_dragEnd = location.x;
    }
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    m_theta -= (m_dragEnd - m_dragStart) * 0.01f;
    m_dragging = false;
}

- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint current = [touch locationInView: self];
    m_dragEnd = current.x;
}

@end
