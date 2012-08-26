#import "Interfaces.hpp"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView {
@private
    IApplicationEngine* m_applicationEngine;
    IRenderingEngine* m_renderingEngine;
    IResourceManager* m_resourceManager;
    EAGLContext* m_context;
    float m_timestamp;
    UISegmentedControl* m_filterChooser;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
