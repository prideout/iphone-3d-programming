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
            fromDrawable: eaglLayer];
                
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
        
        // Create and configure the UIKit control:
        
        NSArray* labels = [NSArray arrayWithObjects:@"Nearest",
                                                    @"Bilinear",
                                                    @"Trilinear", nil];
        
		m_filterChooser = [[[UISegmentedControl alloc] initWithItems:labels] autorelease];
        m_filterChooser.segmentedControlStyle = UISegmentedControlStyleBar;
        m_filterChooser.selectedSegmentIndex = 0;
        //m_filterChooser.alpha = 0.5f;
        
		[m_filterChooser addTarget:self
                         action:@selector(changeFilter:)
                         forControlEvents:UIControlEventValueChanged];
        
        // Add the control to GLView's children:
        
		[self addSubview:m_filterChooser];
        
        // Position the UIKit control:
        
        const int ScreenWidth = CGRectGetWidth(frame);
        const int ScreenHeight = CGRectGetHeight(frame);
        const int Margin = 10;
        
        CGRect controlFrame = m_filterChooser.frame;
        controlFrame.origin.x = ScreenWidth / 2 - controlFrame.size.width / 2;
        controlFrame.origin.y = ScreenHeight - controlFrame.size.height - Margin;
        m_filterChooser.frame = controlFrame;
    }
    return self;
}

- (void) changeFilter: (id) sender
{
    TextureFilter filter = (TextureFilter) [sender selectedSegmentIndex];
    m_renderingEngine->SetFilter(filter);
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

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    m_applicationEngine->OnFingerDown(ivec2(location.x, location.y));
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    m_applicationEngine->OnFingerUp(ivec2(location.x, location.y));
}

- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint previous  = [touch previousLocationInView: self];
    CGPoint current = [touch locationInView: self];
    m_applicationEngine->OnFingerMove(ivec2(previous.x, previous.y),
                                      ivec2(current.x, current.y));
}

@end
