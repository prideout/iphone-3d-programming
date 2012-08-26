#!/usr/bin/python

import cairo
import os
import math
from PIL import Image

# Dump out the Kerning Table to an XML file:
# http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6kern.html
# http://sourceforge.net/projects/fonttools/
if False:
    from fontTools import ttLib
    ttf = ttLib.TTFont("/Library/Fonts/Apple Chancery.ttf")
    ttf.saveXML("ChanceryKerning.xml", tables=['kern'])

# Create a Cairo image surface:
textureWidth, textureHeight = 256, 128
imagesize = (textureWidth,textureHeight)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
cr = cairo.Context(surface)

# Choose a font from /Library/Fonts
cr.select_font_face("Apple Chancery", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
cr.set_font_size(20)
cr.set_source_rgb(1,1,1)

# Create a string for appending the glyph metrics to the texture file:
glyphs = '''
struct GlyphPosition {
    int X;
    int Y;
};\n
struct GlyphMetrics {
    int XBearing;
    int YBearing;
    int Width;
    int Height;
    int XAdvance;
    int YAdvance;
};\n
struct Glyph {
    GlyphPosition Position;
    GlyphMetrics Metrics;
};\n
static const Glyph Glyphs[] = {\n'''

# Render all the glyphs and write out their metrics:
x = 0
y = 0
maxHeight = 0
for i in range(1, 98):
    glyph = (i, 0, 0)
    extents = cr.glyph_extents([glyph])
    x_bearing, y_bearing, width, height, x_advance, y_advance = extents
    maxHeight = max(height, maxHeight)
    if x - x_bearing + width >= textureWidth:
        x = 0
        y += maxHeight
        maxHeight = 0
    glyphs += '    {{ %d, %d }, ' % (x, y)
    glyphs += '{ %d, %d, %d, %d, %d, %d }},\n' % extents
    cr.translate(x-x_bearing,y-y_bearing)
    cr.glyph_path([glyph])
    cr.fill()
    cr.translate(x_bearing-x,y_bearing-y)
    x += width
glyphs += '};\n'

# Save the PNG image and open it up for a quick preview:
surface.write_to_png("GlyphsTexture.png")
os.system("open GlyphsTexture.png")

# Extract the alpha channel:
image = Image.open("GlyphsTexture.png")
image.split()[3].save("GlyphsTexture.png")

# Serialize the image data to a C header file:
path = '/Users/prideout/Samples/PowerVR-ES2/Utilities/PVRTexTool/PVRTexToolCL/MacOS/'
os.system(path + 'PVRTexTool -h -yflip1 -fOGL8 -iGlyphsTexture.png')

# Append to the header file:
headerFile = open('GlyphsTexture.h', 'a')
headerFile.write(glyphs)
headerFile.close()
