#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image

def showImage(image):
    if uname()[0] == 'Linux':
        system("gnome-open " + image)
    else:
        system("open " + image)

def text(str, x, y):
    global cr
    x_bearing, y_bearing, width, height = cr.text_extents(str)[:4]
    cr.move_to(x - width / 2 - x_bearing, y - height / 2 - y_bearing)
    cr.show_text(str)

# Create Text:

imagesize = (256,128)
viewport = (256,128)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.select_font_face("Helvetica", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
cr.set_font_size(24)
cr.set_source_rgb(1,1,1)
text("Alpha Blended", 128, 21)
text("Alpha Tested", 128, 64)
text("Distance Field", 128, 106)

if False:
    cr.move_to(0, 42), cr.line_to(256,42)
    cr.move_to(0, 84), cr.line_to(256,84)
    cr.stroke()

surface.write_to_png("Text.png")
alphaChannel = Image.open("Text.png").split()[3]
alphaChannel.save("Text.png")
showImage("Text.png")

# Create SmallText:

imagesize = (256,128)
viewport = (256,128)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.select_font_face("Helvetica", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
cr.set_font_size(20)
cr.set_source_rgb(1,1,1)
text("Smooth", 128, 16)
text("Outline", 128, 48)
text("Glow", 128, 80)
text("Shadow", 128, 112)

if False:
    for i in xrange(4):
        cr.move_to(0, i*32), cr.line_to(256,i*32)
    cr.stroke()

surface.write_to_png("SmallText.png")
alphaChannel = Image.open("SmallText.png").split()[3]
alphaChannel.save("SmallText.png")

showImage("SmallText.png")

# Convert to header files:

if True:
    path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/PVRTexTool/PVRTexToolCL/MacOS/'
    system(path + 'PVRTexTool -h -yflip1 -fOGL8 -iText.png')
    system(path + 'PVRTexTool -h -yflip1 -fOGL8 -iSmallText.png')
    system(path + 'PVRTexTool -h -yflip1 -fOGL565 -iTile.png')

