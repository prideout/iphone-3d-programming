#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"

using namespace std;

class ResourceManager : public IResourceManager {
public:
    ResourceManager()
    {
        m_imageData = 0;
    }
    TextureDescription LoadImagePot(const string& file)
    {
        NSString* basePath = [NSString stringWithUTF8String:file.c_str()];
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        NSString* fullPath = [resourcePath stringByAppendingPathComponent:basePath];
        UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
        
        TextureDescription description;
        description.OriginalSize.x = CGImageGetWidth(uiImage.CGImage);
        description.OriginalSize.y = CGImageGetHeight(uiImage.CGImage);
        description.Size.x = NextPot(description.OriginalSize.x);
        description.Size.y = NextPot(description.OriginalSize.y);
        description.BitsPerComponent = 8;
        description.Format = TextureFormatRgba;
        
        int bpp = description.BitsPerComponent / 2;
        int byteCount = description.Size.x * description.Size.y * bpp;
        unsigned char* data = (unsigned char*) calloc(byteCount, 1);
        
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGBitmapInfo bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
        CGContextRef context = CGBitmapContextCreate(data,
                                                     description.Size.x,
                                                     description.Size.y,
                                                     description.BitsPerComponent,
                                                     bpp * description.Size.x,
                                                     colorSpace,
                                                     bitmapInfo);
        CGColorSpaceRelease(colorSpace);
        CGRect rect = CGRectMake(0, 0, description.Size.x, description.Size.y);
        CGContextDrawImage(context, rect, uiImage.CGImage);
        CGContextRelease(context);
        
        m_imageData = [NSData dataWithBytesNoCopy:data length:byteCount freeWhenDone:YES];
        return description;
    }
    void* GetImageData()
    {
        return (void*) [m_imageData bytes];
    }
    void UnloadImage()
    {
        m_imageData = 0;
    }

private:
    NSData* m_imageData;
    unsigned int NextPot(unsigned int n)
    {
        n--;
        n |= n >> 1; n |= n >> 2;
        n |= n >> 4; n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }
};

IResourceManager* CreateResourceManager()
{
    return new ResourceManager();
}
