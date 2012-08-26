#import "IRenderingEngine.hpp"
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView {
@private
    IRenderingEngine* m_renderingEngine;
    EAGLContext* m_context;
    float m_timestamp;
}

- (void) drawView: (CADisplayLink*) displayLink;
- (void) didRotate: (NSNotification*) notification;

@end
