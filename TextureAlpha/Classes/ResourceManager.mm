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
    string GetResourcePath() const
    {
        NSString* bundlePath =[[NSBundle mainBundle] resourcePath];
        return [bundlePath UTF8String];
    }
    TextureDescription LoadImage(const string& file)
    {
        NSString* basePath = [NSString stringWithUTF8String:file.c_str()];
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        NSString* fullPath = [resourcePath stringByAppendingPathComponent:basePath];
        UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
                
        TextureDescription description;
        description.Size.x = CGImageGetWidth(uiImage.CGImage);
        description.Size.y = CGImageGetHeight(uiImage.CGImage);
        description.BitsPerComponent = 8;
        description.Format = TextureFormatRgba;
        description.MipCount = 1;

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
    TextureDescription LoadPngImage(const string& file)
    {
        NSString* basePath = [NSString stringWithUTF8String:file.c_str()];
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        NSString* fullPath = [resourcePath stringByAppendingPathComponent:basePath];

        NSLog(@"Loading PNG image %s...", fullPath);

        UIImage* uiImage = [UIImage imageWithContentsOfFile:fullPath];
        CGImageRef cgImage = uiImage.CGImage;

        CFDataRef dataRef = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
        m_imageData = [NSData dataWithData:(NSData*) dataRef];
        CFRelease(dataRef);
        
        TextureDescription description;
        description.Size.x = CGImageGetWidth(cgImage);
        description.Size.y = CGImageGetHeight(cgImage);
        bool hasAlpha = CGImageGetAlphaInfo(cgImage) != kCGImageAlphaNone;
        CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
        switch (CGColorSpaceGetModel(colorSpace)) {
            case kCGColorSpaceModelMonochrome:
                description.Format = hasAlpha ? TextureFormatGrayAlpha : TextureFormatGray;
                break;
            case kCGColorSpaceModelRGB:
                description.Format = hasAlpha ? TextureFormatRgba : TextureFormatRgb;
                break;
            default:
                assert(!"Unsupported color space.");
                break;
        }
        description.BitsPerComponent = CGImageGetBitsPerComponent(cgImage);

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
};

IResourceManager* CreateResourceManager()
{
    return new ResourceManager();
}
