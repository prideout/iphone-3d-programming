#import "HelloArrowAppDelegate.h"

@implementation HelloArrowAppDelegate

- (void) applicationDidFinishLaunching: (UIApplication*) application
{
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    
    m_window = [[UIWindow alloc] initWithFrame: screenBounds];
    m_view = [[GLView alloc] initWithFrame: screenBounds];
    
    [m_window addSubview: m_view];
    [m_window makeKeyAndVisible];
}

- (void) dealloc
{
    [m_view release];
    [m_window release];
    [super dealloc];
}

@end
