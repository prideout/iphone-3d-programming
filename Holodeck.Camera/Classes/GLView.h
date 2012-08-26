#import "Interfaces.hpp"
#import "AccelerometerFilter.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreLocation/CoreLocation.h>

@interface GLView : UIView <UIImagePickerControllerDelegate,
                            UINavigationControllerDelegate,
                            CLLocationManagerDelegate,
                            UIAccelerometerDelegate> {
@private
    IRenderingEngine* m_renderingEngine;
    IResourceManager* m_resourceManager;
    EAGLContext* m_context;
    CLLocationManager* m_locationManager;
    AccelerometerFilter* m_filter;
    UIViewController* m_viewController;
    bool m_cameraSupported;
    bool m_paused;
    float m_theta;
    float m_phi;
    vec2 m_velocity;
    ButtonMask m_visibleButtons;
    float m_timestamp;
}

- (void) drawView: (CADisplayLink*) displayLink;

@end
