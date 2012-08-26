#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance
import fileinput

def ShowImage(image):
    if uname()[0] == 'Linux': system('gnome-open ' + image)
    elif uname()[0] == 'Darwin': system('open ' + image)
    else: system('start ' + image)

path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
textool565 = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL565 -i'
textool8 = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL8 -i'

imagesize = (64,64)
viewport = imagesize
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)
cr.arc(viewport[0]/2.0,viewport[1]/2.0,viewport[0] * 0.4,0,math.pi * 2)
cr.set_source_rgb(1,1,1)
cr.fill()
surface.write_to_png("Circle.png")
ShowImage('Circle.png')

system(path + textool565 + "TombWindow.jpg")
system(path + textool8 + "Circle.png")

images = []
images.append('TombWindow.h')
images.append('Circle.h')

fromStr = "static const unsigned long" 
toStr = "const unsigned int" 

for filename in images:
	for line in fileinput.FileInput(filename, inplace=1):
		print line.replace(fromStr, toStr),
