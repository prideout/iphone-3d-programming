#import "Interfaces.hpp"
#import "AccelerometerFilter.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView <UIAccelerometerDelegate> {
@private
    IApplicationEngine* m_applicationEngine;
    IRenderingEngine* m_renderingEngine;
    EAGLContext* m_context;
    float m_timestamp;
    AccelerometerFilter* m_filter;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
