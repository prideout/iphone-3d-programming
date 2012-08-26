#import "Interfaces.hpp"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView <UIAccelerometerDelegate> {
@private
    IRenderingEngine* m_renderingEngine;
    EAGLContext* m_context;
    float m_timestamp;
    float m_theta;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
