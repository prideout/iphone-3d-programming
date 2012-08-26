#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <string>
#import <iostream>
#import "Interfaces.hpp"
#import "../PowerVR/PVRTTexture.h"

using namespace std;

class ResourceManager : public IResourceManager {
public:
    ResourceManager()
    {
        m_imageData = 0;
    }
    TextureDescription LoadImagePot(const string& file)
    {
        if (file.find(".pvr") != string::npos)
            return LoadPvrImage(file);

        m_hasPvrHeader = false;

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
    TextureDescription LoadPvrImage(const string& file)
    {
        NSString* basePath = [NSString stringWithUTF8String:file.c_str()];
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        NSString* fullPath = [resourcePath stringByAppendingPathComponent:basePath];
        
        m_imageData = [NSData dataWithContentsOfFile:fullPath];
        m_hasPvrHeader = true;
        PVR_Texture_Header* header = (PVR_Texture_Header*) [m_imageData bytes];
        bool hasAlpha = header->dwAlphaBitMask ? true : false;
        
        TextureDescription description;
        switch (header->dwpfFlags & PVRTEX_PIXELTYPE) {
            case OGL_AI_88:
                description.Format = TextureFormatGrayAlpha;
                description.BitsPerComponent = 16;
                break;
            case OGL_I_8:
                description.Format = TextureFormatGray;
                description.BitsPerComponent = 8;
                break;
            case OGL_RGBA_8888:
                description.Format = TextureFormatRgba;
                description.BitsPerComponent = 8;
                break;
            case OGL_RGB_565:
                description.Format = TextureFormat565;
                break;
            case OGL_RGBA_5551:
                description.Format = TextureFormat5551;
                break;
            case OGL_RGBA_4444:
                description.Format = TextureFormatRgba;
                description.BitsPerComponent = 4;
                break;
            case OGL_PVRTC2:    
                description.Format = hasAlpha ? TextureFormatPvrtcRgba2 :
                TextureFormatPvrtcRgb2;
                break;
            case OGL_PVRTC4:
                description.Format = hasAlpha ? TextureFormatPvrtcRgba4 :
                TextureFormatPvrtcRgb4;
                break;
            default:
                assert(!"Unsupported PVR image.");
                break;
        }
        
        description.Size.x = header->dwWidth;
        description.Size.y = header->dwHeight;
        description.MipCount = header->dwMipMapCount;
        return description;
    }
    void* GetImageData()
    {
        if (!m_hasPvrHeader)
            return (void*) [m_imageData bytes];
        
        PVR_Texture_Header* header = (PVR_Texture_Header*) [m_imageData bytes];
        char* data = (char*) [m_imageData bytes];
        unsigned int headerSize = header->dwHeaderSize;
        return data + headerSize;
    }
    void UnloadImage()
    {
        m_imageData = 0;
    }

private:
    NSData* m_imageData;
    bool m_hasPvrHeader;
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
