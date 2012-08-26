//
//  SimpleWireframeAppDelegate.m
//  SimpleWireframe
//
//  Created by Brian Jepson on 2/23/10.
//  Copyright O'Reilly Media 2010. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate


- (void) applicationDidFinishLaunching: (UIApplication*) application
{
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    
    m_window = [[UIWindow alloc] initWithFrame: screenBounds];
    m_view = [[GLView alloc] initWithFrame: screenBounds];
    
    [m_window addSubview: m_view];
    [m_window makeKeyAndVisible];
}


- (void)dealloc {
    [m_view release];
    [m_window release];
    [super dealloc];
}


@end
