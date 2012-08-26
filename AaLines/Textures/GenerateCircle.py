#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance

def showImage(image):
    if uname()[0] == 'Linux':
        system("gnome-open " + image)
    else:
        system("open " + image)

imagesize = (16,16)
viewport = imagesize
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.arc(viewport[0]/2.0,viewport[1]/2.0,viewport[0] * 0.4,0,math.pi * 2)
cr.fill()

surface.write_to_png("Circle.png")
alphaChannel = Image.open("Circle.png").split()[3]
alphaChannel.save("Circle.png")
for i in xrange(4):
    alphaChannel = alphaChannel.filter(ImageFilter.BLUR)
#alphaChannel = ImageEnhance.Brightness(alphaChannel).enhance(1.25)

alphaChannel.save("BlurryCircle.png")

# Convert to header files:

if True:
    path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/PVRTexTool/PVRTexToolCL/MacOS/'
    path = '/Users/prideout/Samples/PowerVR-ES2/Utilities/PVRTexTool/PVRTexToolCL/MacOS/'
    system(path + 'PVRTexTool -h -yflip1 -fOGL8 -iCircle.png')
    system(path + 'PVRTexTool -h -yflip1 -fOGL8 -iBlurryCircle.png')
    system(path + 'PVRTexTool -h -yflip1 -fOGL565 -iTile.png')

