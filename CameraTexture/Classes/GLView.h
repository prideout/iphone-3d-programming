#import "Interfaces.hpp"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@interface GLView : UIView <UIImagePickerControllerDelegate,
                            UINavigationControllerDelegate> {
@private
    IRenderingEngine* m_renderingEngine;
    IResourceManager* m_resourceManager;
    EAGLContext* m_context;
    UIViewController* m_viewController;
    bool m_paused;
    float m_zScale;
    float m_xRotation;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
