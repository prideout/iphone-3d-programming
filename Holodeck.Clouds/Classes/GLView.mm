#import "GLView.h"

#define GL_RENDERBUFFER 0x8d41

@implementation GLView

+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id) initWithFrame: (CGRect) frame
{
    m_paused = false;
    m_theta = 0;
    m_phi = 0;
    m_velocity = vec2(0, 0);
    m_visibleButtons = 0;

    if (self = [super initWithFrame:frame]) {
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
        m_renderingEngine = CreateRenderingEngine(m_resourceManager);

        m_locationManager = [[CLLocationManager alloc] init];

#if TARGET_IPHONE_SIMULATOR
        BOOL compassSupported = NO;
        BOOL accelSupported = NO;
#else
        BOOL compassSupported = m_locationManager.headingAvailable;
        BOOL accelSupported = YES;
#endif
        
        if (compassSupported) {
            NSLog(@"Compass is supported.");
            m_locationManager.headingFilter = kCLHeadingFilterNone;
            m_locationManager.delegate = self;
            [m_locationManager startUpdatingHeading];
        } else {
            NSLog(@"Compass is NOT supported.");
            m_visibleButtons |= ButtonFlagsShowHorizontal;
        }
        
        if (accelSupported) {
            NSLog(@"Accelerometer is supported.");
            float updateFrequency = 60.0f;
            m_filter = [[LowpassFilter alloc] initWithSampleRate:updateFrequency
                                              cutoffFrequency:5.0];
            m_filter.adaptive = YES;

            [[UIAccelerometer sharedAccelerometer] setUpdateInterval:1.0 / updateFrequency];
            [[UIAccelerometer sharedAccelerometer] setDelegate:self];
        } else {
            NSLog(@"Accelerometer is NOT supported.");
            m_visibleButtons |= ButtonFlagsShowVertical;
        }

        [m_context
            renderbufferStorage:GL_RENDERBUFFER
            fromDrawable: eaglLayer];
        
        m_timestamp = CACurrentMediaTime();

        m_renderingEngine->Initialize();
        [self drawView:nil];
        
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
    if (m_paused)
        return;
    
    if (displayLink != nil) {
        const float speed = 30;
        float elapsedSeconds = displayLink.timestamp - m_timestamp;
        m_timestamp = displayLink.timestamp;
        m_theta -= speed * elapsedSeconds * m_velocity.x;
        m_phi += speed * elapsedSeconds * m_velocity.y;
    }

    ButtonMask buttonFlags = m_visibleButtons;
    if (m_velocity.x < 0) buttonFlags |= ButtonFlagsPressingLeft;
    if (m_velocity.x > 0) buttonFlags |= ButtonFlagsPressingRight;
    if (m_velocity.y < 0) buttonFlags |= ButtonFlagsPressingUp;
    if (m_velocity.y > 0) buttonFlags |= ButtonFlagsPressingDown;
    
    m_renderingEngine->Render(m_theta, m_phi, buttonFlags);
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

bool buttonHit(CGPoint location, int x, int y)
{
    float extent = 32;
    return (location.x > x - extent && location.x < x + extent &&
            location.y > y - extent && location.y < y + extent);
}

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    float delta = 1;

    if (m_visibleButtons & ButtonFlagsShowVertical) {
        if (buttonHit(location, 35, 240))
            m_velocity.y = -delta;
        else if (buttonHit(location, 285, 240))
            m_velocity.y = delta;
    }
    
    if (m_visibleButtons & ButtonFlagsShowHorizontal) {
        if (buttonHit(location, 160, 40))
            m_velocity.x = -delta;
        else if (buttonHit(location, 160, 440))
            m_velocity.x = delta;
    }
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    m_velocity = vec2(0, 0);
}

- (void) locationManager: (CLLocationManager*) manager
         didUpdateHeading: (CLHeading*) heading
{
    // Use magneticHeading rather than trueHeading to avoid usage of GPS:
    CLLocationDirection degrees = heading.magneticHeading;
    m_theta = (float) -degrees;
}

- (void) accelerometer: (UIAccelerometer*) accelerometer
         didAccelerate: (UIAcceleration*) acceleration
{
    [m_filter addAcceleration:acceleration];
    float x = m_filter.x;
    float y = m_filter.z;
    m_phi = atan2(y, -x) * 180.0f / Pi;
}


@end
