#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance
import fileinput

def showImage(image):
    if uname()[0] == 'Linux':
        system("gnome-open " + image)
    else:
        system("open " + image)

imagesize = (512, 1024)
viewport = imagesize
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.set_source_rgb(1,1,1)
cr.arc(imagesize[0]/2,imagesize[1]/2,6000,0,math.pi * 2)
cr.fill()

for x in xrange(50):
	for y in xrange(100):
		radius = 6
		spread = 32
		offset = 16

		cr.set_source_rgb(0,0,0)
		cr.arc(x * spread + offset,y * spread + offset,radius,0,math.pi * 2)
		cr.fill()

surface.write_to_png("HeightMap.png")
img = Image.open("HeightMap.png")
img.save("HeightMap.png")
for i in xrange(2):
    img = img.filter(ImageFilter.BLUR)
img.save("HeightMap.png")
showImage("HeightMap.png")

path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
textool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL888 -i'
normaltool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -b -c2.0 -fOGL888 -i'
filewrap = 'Filewrap/MacOS/Filewrap -h -o '

system(path + normaltool + "HeightMap.png -oTangentSpaceNormals.h")
system(path + textool + "ObjectSpaceNormals.png")

fromStr = "static const unsigned long" 
toStr = "const unsigned int" 

for filename in ['TangentSpaceNormals.h', 'ObjectSpaceNormals.h']:
	for line in fileinput.FileInput(filename, inplace=1):
		print line.replace(fromStr, toStr),
