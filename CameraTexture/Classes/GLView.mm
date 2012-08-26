#import "GLView.h"
#import <OpenGLES/ES2/gl.h> // <-- for GL_RENDERBUFFER only

@implementation GLView

+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id) initWithFrame: (CGRect) frame
{
    m_paused = false;
    m_zScale = 0.5;
    m_xRotation = 0;
    
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
        m_renderingEngine = CreateRenderingEngine(m_resourceManager);
        
        [m_context
            renderbufferStorage:GL_RENDERBUFFER
            fromDrawable: eaglLayer];
        
        m_renderingEngine->Initialize();
        m_renderingEngine->Render(m_zScale, m_xRotation, true);
        [m_context presentRenderbuffer:GL_RENDERBUFFER];
        
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
        float t = displayLink.timestamp / 3;
        int integer = (int) t;
        float fraction = t - integer;
        if (integer % 2) {
            m_xRotation = 360 * fraction;
            m_zScale = 0.5;
        } else {
            m_xRotation = 0;
            m_zScale = 0.5 + sin(fraction * 6 * M_PI) * 0.3;
        }
    }
    
    m_renderingEngine->Render(m_zScale, m_xRotation, false);
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    
    // Return early if touched outside the button's area.
    if (location.y < 395 || location.y > 450 || 
        location.x < 75 || location.x > 245)
        return;

    // Instance the image picker and set up its configuration.
    UIImagePickerController* imagePicker = [[UIImagePickerController alloc] init];
    imagePicker.delegate = self;
    imagePicker.navigationBarHidden = YES;
    imagePicker.toolbarHidden = YES;

    // Enable camera mode if supported, otherwise fall back to the default.
    UIImagePickerControllerSourceType source = UIImagePickerControllerSourceTypeCamera;
    if ([UIImagePickerController isSourceTypeAvailable:source])
        imagePicker.sourceType = source;  

    // Instance the view controller if it doesn't already exist.
    if (m_viewController == 0) {
        m_viewController = [[UIViewController alloc] init];
        m_viewController.view = self;
    }
    
    // Turn off the OpenGL rendering cycle and present the image picker.
    m_paused = true;
    [m_viewController presentModalViewController:imagePicker animated:NO];
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    [m_viewController dismissModalViewControllerAnimated:NO];
    m_paused = false;
    [picker release];
}

- (void)imagePickerController:(UIImagePickerController *) picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    UIImage* image = [info objectForKey:UIImagePickerControllerOriginalImage];
    
    float theta = 0;
    switch (image.imageOrientation) {
        case UIImageOrientationDown: theta =  M_PI; break;
        case UIImageOrientationLeft: theta = M_PI / 2; break;
        case UIImageOrientationRight: theta = -M_PI / 2; break;
    }

    int bpp = 4;
    ivec2 size(256, 256);
    int byteCount = size.x * size.y * bpp;
    unsigned char* data = (unsigned char*) calloc(byteCount, 1);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
    CGContextRef context = CGBitmapContextCreate(data,
                                                 size.x,
                                                 size.y,
                                                 8,
                                                 bpp * size.x,
                                                 colorSpace,
                                                 bitmapInfo);
    CGColorSpaceRelease(colorSpace);
    CGRect rect = CGRectMake(0, 0, size.x, size.y);
    CGContextTranslateCTM(context, size.x / 2, size.y / 2);
    CGContextRotateCTM(context, theta);
    CGContextTranslateCTM(context, -size.x / 2, -size.y / 2);
    CGContextDrawImage(context, rect, image.CGImage);
    
    TextureDescription description;
    description.Size = size;
    description.OriginalSize.x = CGImageGetWidth(image.CGImage);
    description.OriginalSize.y = CGImageGetHeight(image.CGImage);
    description.Format = TextureFormatRgba;
    description.BitsPerComponent = 8;
    
    m_renderingEngine->LoadCameraTexture(description, data);
    m_renderingEngine->Render(m_zScale, m_xRotation, true);
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
    
    CGContextRelease(context);
    free(data);
    
    [m_viewController dismissModalViewControllerAnimated:NO];
    m_paused = false;
    [picker release];
}

@end
