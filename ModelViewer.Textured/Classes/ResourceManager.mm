#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"

using namespace std;

namespace Darwin {
    
class ResourceManager : public IResourceManager {
public:
    string GetResourcePath() const
    {
        NSString* bundlePath =[[NSBundle mainBundle] resourcePath];
        return [bundlePath UTF8String];
    }
    void LoadPngImage(const string& name)
    {
        NSString* basePath = [[NSString alloc] initWithUTF8String:name.c_str()];
        NSBundle* mainBundle = [NSBundle mainBundle];
        NSString* fullPath = [mainBundle pathForResource:basePath ofType:@"png"];
        UIImage* uiImage = [[UIImage alloc] initWithContentsOfFile:fullPath];
        CGImageRef cgImage = uiImage.CGImage;
        m_imageSize.x = CGImageGetWidth(cgImage);
        m_imageSize.y = CGImageGetHeight(cgImage);
        m_imageData = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
        [uiImage release];
        [basePath release];
    }
    void* GetImageData()
    {
        return (void*) CFDataGetBytePtr(m_imageData);
    }
    ivec2 GetImageSize()
    {
        return m_imageSize;
    }
    void UnloadImage()
    {
        CFRelease(m_imageData);
    }
private:
    ivec2 m_imageSize;
    CFDataRef m_imageData;
};

IResourceManager* CreateResourceManager()
{
    return new ResourceManager();
}

}